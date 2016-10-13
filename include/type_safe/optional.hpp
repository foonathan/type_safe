// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_OPTIONAL_HPP_INCLUDED
#define TYPE_SAFE_OPTIONAL_HPP_INCLUDED

#include <functional>
#include <new>
#include <type_traits>

#include <type_safe/detail/assert.hpp>

namespace type_safe
{
    template <class StoragePolicy>
    class basic_optional;

    /// \exclude
    namespace detail
    {
        // std::is_assignable but without user-defined conversions
        template <typename T, typename Arg>
        struct is_direct_assignable
        {
            template <typename U>
            struct consume_udc
            {
                operator U() const;
            };

            template <typename U>
            static std::true_type check(
                decltype(std::declval<T&>() = std::declval<consume_udc<U>>(), 0)*);

            template <typename U>
            static std::false_type check(...);

            static constexpr bool value = decltype(check<Arg>(0))::value;
        };

        //=== is_nothrow_swappable ===//
        template <typename T>
        struct is_nothrow_swappable
        {
            template <typename U>
            static auto adl_swap(int, U& a, U& b) noexcept(noexcept(swap(a, b)))
                -> decltype(swap(a, b));

            template <typename U>
            static auto adl_swap(short, U& a, U& b) noexcept(noexcept(std::swap(a, b)))
                -> decltype(std::swap(a, b));

            static constexpr bool value =
                noexcept(adl_swap(0, std::declval<T&>(), std::declval<T&>()));
        };

        //=== is_optional ===//
        template <typename T>
        struct is_optional_impl : std::false_type
        {
        };

        template <class StoragePolicy>
        struct is_optional_impl<basic_optional<StoragePolicy>> : std::true_type
        {
        };

        template <typename T>
        using is_optional = is_optional_impl<typename std::decay<T>::type>;

        //=== unwrapping ===//
        template <class Optional>
        using need_unwrap_optional = is_optional<typename std::decay<Optional>::type::value_type>;

        template <class Optional>
        using unwrap_optional_impl =
            typename std::conditional<is_optional<typename Optional::value_type>::value,
                                      typename Optional::value_type, Optional>::type;

        template <class Optional>
        using unwrap_optional_t =
            typename std::decay<unwrap_optional_impl<typename std::decay<Optional>::type>>::type;

        template <class Optional>
        unwrap_optional_t<Optional> unwrap_optional(std::true_type, Optional&& opt)
        {
            return opt.has_value() ? std::forward<Optional>(opt).value() :
                                     unwrap_optional_t<Optional>{};
        }

        template <class Optional>
        unwrap_optional_t<Optional> unwrap_optional(std::false_type, Optional&& opt)
        {
            return std::forward<Optional>(opt);
        }

        template <class Optional>
        unwrap_optional_t<Optional> unwrap_optional(Optional&& opt)
        {
            return unwrap_optional(need_unwrap_optional<Optional>{}, std::forward<Optional>(opt));
        }
    } // namespace detail

    //=== basic_optional ===//
    /// Tag type to mark a [type_safe::basic_optional<StoragePolicy>]() without a value.
    struct nullopt_t
    {
        constexpr nullopt_t()
        {
        }
    };

    /// Tag object of type [type_safe::nullopt_t]().
    constexpr nullopt_t nullopt;

    /// An optional type, i.e. a type that may or may not be there.
    ///
    /// It is similar to [std::optional<T>]() but lacks some functions and provides some others.
    /// It can be in one of two states: it contains a value of a certain type or it does not (it is "empty").
    ///
    /// The storage itself is managed via the `StoragePolicy`.
    /// It must provide the following members:
    /// * Typedef `value_type` - the type stored in the optional
    /// * Typedef `(const_)lvalue_reference` - `const` lvalue reference type
    /// * Typedef `(const_)rvalue_reference` - `const` rvalue reference type
    /// * Template alias `rebind<U>` - the same policy for a different type
    /// * `StoragePolicy() noexcept` - a no-throw default constructor that initializes it in the "empty" state
    /// * `void create_value(Args&&... args)` - creates a value by forwarding the arguments to its constructor
    /// * `void destroy_value() noexcept` - calls the destructor of the value, afterwards the storage is "empty"
    /// * `bool has_value() const noexcept` - returns whether or not there is a value, i.e. `create_value()` has been called but `destroy_value()` has not
    /// * `U get_value() (const)& noexcept` - returns a reference to the stored value, U is one of the `XXX_reference` typedefs
    /// * `U get_value() (const)&& noexcept` - returns a reference to the stored value, U is one of the `XXX_reference` typedefs
    /// * `U get_value_or(T&& val) [const&/&&]` - returns either `get_value()` or `val`
    template <class StoragePolicy>
    class basic_optional
    {
    public:
        using storage    = StoragePolicy;
        using value_type = typename storage::value_type;

        template <typename U>
        using rebind = basic_optional<typename StoragePolicy::template rebind<U>>;

    private:
        storage policy_;

    public:
        //=== constructors/destructors/assignment/swap ===//
        /// \effects Creates it without a value.
        basic_optional() noexcept = default;

        /// \effects Same as the default constructor.
        basic_optional(nullopt_t) noexcept
        {
        }

        /// \effects Creates it with a value by forwarding `value`.
        /// \throws Anything thrown by the constructor of `value_type`.
        /// \requires The `create_value()` function of the `StoragePolicy` must accept `value`.
        template <typename T>
        basic_optional(T&& value, decltype(policy_.create_value(std::forward<T>(value)), 0) = 0)
        {
            policy_.create_value(std::forward<T>(value));
        }

        /// \effects Copy constructor:
        /// If `other` does not have a value, it will be created without a value as well.
        /// If `other` has a value, it will be created with a value by copying `other.value()`.
        /// \throws Anything thrown by the copy constructor of `value_type` if `other` has a value.
        basic_optional(const basic_optional& other)
        {
            if (other.has_value())
                policy_.create_value(other.value());
        }

        /// \effects Move constructor:
        /// If `other` does not have a value, it will be created without a value as well.
        /// If `other` has a value, it will be created with a value by moving `other.value()`.
        /// \throws Anything thrown by the move constructor of `value_type` if `other` has a value.
        /// \notes `other` will still have a value after the move operation,
        /// it is just in a moved-from state.
        basic_optional(basic_optional&& other) noexcept(
            std::is_nothrow_move_constructible<value_type>::value)
        {
            if (other.has_value())
                policy_.create_value(std::move(other).value());
        }

        /// \effects If it has a value, it will be destroyed.
        ~basic_optional() noexcept
        {
            reset();
        }

        /// \effects Same as `reset()`.
        basic_optional& operator=(nullopt_t) noexcept
        {
            reset();
            return *this;
        }

        /// \effects Same as `emplace(std::forward<T>(t))`.
        /// \requires The call to `emplace()` must be well-formed.
        template <typename T>
        auto operator=(T&& value)
            -> decltype(this->policy_.create_value(std::forward<T>(value)), *this)
        {
            emplace(std::forward<T>(value));
            return *this;
        }

        /// \effects Copy assignment operator:
        /// If `other` has a value, calls `emplace(other.value())` (this will always trigger the single parameter version).
        /// Otherwise calls `reset()`.
        /// \throws Anything thrown by the call to `emplace()`.
        basic_optional& operator=(const basic_optional& other)
        {
            if (other.has_value())
                emplace(other.value());
            else
                reset();
            return *this;
        }

        /// \effects Move assignment operator:
        /// If `other` has a value, calls `emplace(std::move(other).value())` (this will always trigger the single parameter version).
        /// Otherwise calls `reset()`.
        /// \throws Anything thrown by the call to `emplace()`.
        basic_optional& operator=(basic_optional&& other) noexcept(
            std::is_nothrow_move_constructible<value_type>::value&&
                std::is_nothrow_move_assignable<value_type>::value)
        {
            if (other.has_value())
                emplace(std::move(other).value());
            else
                reset();
            return *this;
        }

        /// \effects Swap.
        /// If both `a` and `b` have values, swaps the values with their swap function.
        /// Otherwise, if only one of them have a value, moves that value to the other one and makes the moved-from empty.
        /// Otherwise, if both are empty, does nothing.
        /// \throws Anything thrown by the move construction or swap.
        friend void swap(basic_optional& a, basic_optional& b) noexcept(
            std::is_nothrow_move_constructible<value_type>::value&&
                detail::is_nothrow_swappable<value_type>::value)
        {
            if (a.has_value() && b.has_value())
            {
                using std::swap;
                swap(a.value(), b.value());
            }
            else if (a.has_value())
            {
                b.policy_.create_value(std::move(a.value()));
                a.reset();
            }
            else if (b.has_value())
            {
                a.policy_.create_value(std::move(b.value()));
                b.reset();
            }
        }

        //=== modifiers ===//
        /// \effects Destroys the value by calling its destructor, if there is any stored.
        /// Afterwards `has_value()` will return `false`.
        void reset() noexcept
        {
            if (has_value())
                policy_.destroy_value();
        }

        /// \effects First destroys any old value like `reset()`.
        /// Then creates the value by perfectly forwarding `args...` to the constructor of `value_type`.
        /// \throws Anything thrown by the constructor of `value_type`.
        /// If this function is left by an exception, the optional will be empty.
        /// \notes If the `create_value()` function of the `StoragePolicy` does not accept the arguments,
        /// this function will not participate in overload resolution.
        template <typename... Args>
        auto emplace(Args&&... args) noexcept(
            std::is_nothrow_constructible<value_type, Args...>::value)
            -> decltype(this->policy_.create_value(std::forward<Args>(args)...))
        {
            reset();
            policy_.create_value(std::forward<Args>(args)...);
        }

        /// \effects If `has_value()` is `false` creates it by calling the constructor with `arg` perfectly forwarded.
        /// Otherwise assigns a perfectly forwarded `arg` to `value()`.
        /// \throws Anything thrown by the constructor or assignment operator chosen.
        /// \notes This function does not participate in overload resolution
        /// unless there is an `operator=` that takes `arg` without an implicit user-defined conversion
        /// and the `create_value()` function of the `StoragePolicy` accepts the argument.
        template <
            typename Arg,
            typename =
                typename std::enable_if<detail::is_direct_assignable<value_type, Arg>::value>::type>
        auto emplace(Arg&& arg) noexcept(std::is_nothrow_constructible<value_type, Arg>::value&&
                                             std::is_nothrow_assignable<value_type, Arg>::value)
            -> decltype(this->policy_.create_value(std::forward<Arg>(arg)))
        {
            if (!has_value())
                policy_.create_value(std::forward<Arg>(arg));
            else
                value() = std::forward<Arg>(arg);
        }

        //=== observers ===//
        /// \returns The same as `has_value()`.
        explicit operator bool() const noexcept
        {
            return has_value();
        }

        /// \returns Whether or not the optional has a value.
        bool has_value() const noexcept
        {
            return policy_.has_value();
        }

        /// \returns A reference to the stored value.
        /// \requires `has_value() == true`.
        auto value() & noexcept -> decltype(policy_.get_value())
        {
            DEBUG_ASSERT(has_value(), detail::assert_handler{});
            return policy_.get_value();
        }

        /// \returns A `const` reference to the stored value.
        /// \requires `has_value() == true`.
        auto value() const & noexcept -> decltype(policy_.get_value())
        {
            DEBUG_ASSERT(has_value(), detail::assert_handler{});
            return policy_.get_value();
        }

        /// \returns An rvalue reference to the stored value.
        /// \requires `has_value() == true`.
        auto value() && noexcept -> decltype(std::move(policy_).get_value())
        {
            DEBUG_ASSERT(has_value(), detail::assert_handler{});
            return std::move(policy_).get_value();
        }

        /// \returns An rvalue reference to the stored value.
        /// \requires `has_value() == true`.
        auto value() const && noexcept -> decltype(std::move(policy_).get_value())
        {
            DEBUG_ASSERT(has_value(), detail::assert_handler{});
            return std::move(policy_).get_value();
        }

        /// \returns If it has a value, `value()`, otherwise `u` converted to the same type as `value()`.
        /// \requires `u` must be valid argument to the `value_or()` function of the `StoragePolicy`.
        /// \notes Depending on the `StoragePolicy`, this either returns a decayed type or a reference.
        template <typename U>
        auto value_or(U&& u) const& -> decltype(policy_.get_value_or(std::forward<U>(u)))
        {
            return policy_.get_value_or(std::forward<U>(u));
        }

        /// \returns If it has a value, `value()`, otherwise `u` converted to the same type as `value()`.
        /// \requires `u` must be valid argument to the `value_or()` function of the `StoragePolicy`.
        /// \notes Depending on the `StoragePolicy`, this either returns a decayed type or a reference.
        template <typename U>
        auto value_or(U&& u) && -> decltype(std::move(policy_).get_value_or(std::forward<U>(u)))
        {
            return std::move(policy_).get_value_or(std::forward<U>(u));
        }

        //=== factories ===//
        /// \returns If `value_type` is a `basic_optional` itself, returns a copy of that optional
        /// or a null optional of that type if `has_value()` is `false`.
        /// Otherwise returns a copy of `*this`.
        /// \requires `value_type` must be copy constructible.
        template <typename T = value_type,
                  typename std::enable_if<std::is_copy_constructible<T>::value, int>::type = 0>
        detail::unwrap_optional_t<basic_optional<StoragePolicy>> unwrap() const&
        {
            return detail::unwrap_optional(*this);
        }

        /// \returns If `value_type` is a `basic_optional` itself, returns a copy of that optional by moving
        /// or a null optional of that type if `has_value()` is `false`.
        /// Otherwise returns a copy of `*this` by moving.
        /// \requires `value_type` must be move constructible.
        template <typename T = value_type,
                  typename std::enable_if<std::is_move_constructible<T>::value, int>::type = 0>
        detail::unwrap_optional_t<basic_optional<StoragePolicy>> unwrap() &&
        {
            return detail::unwrap_optional(*this);
        }

        /// \returns The return type is the `basic_optional` rebound to the return type of the function when called with `const value_type&`.
        /// If `has_value()` is `true`, returns the new optional with result of `std::forward<Func>(f)(std::move(value))`,
        /// otherwise returns an empty optional.
        /// \requires `f` must be callable with `const value_type&`.
        template <typename Func>
        auto map(Func&& f) const& -> rebind<decltype(std::forward<Func>(f)(this->value()))>
        {
            if (has_value())
                return std::forward<Func>(f)(value());
            else
                return nullopt;
        }

        /// \returns The return type is the `basic_optional` rebound to the return type of the function when called with `value_type&&`.
        /// If `has_value()` is `true`, returns the new optional with result of `std::forward<Func>(f)(std::move(value))`,
        /// otherwise returns an empty optional.
        /// \notes It will move the `value()` to the function.
        /// \requires `f` must be callable with `value_type&&`.
        template <typename Func>
        auto map(Func&& f) && -> rebind<decltype(std::forward<Func>(f)(std::move(this->value())))>
        {
            if (has_value())
                return std::forward<Func>(f)(std::move(value()));
            else
                return nullopt;
        }

        /// \returns The same as `map(std::forward<Func>(f)).unwrap()`.
        /// \notes This is useful for functions that return an optional type itself,
        /// the optional will be "flattened" properly.
        template <typename Func>
        auto bind(Func&& f)
            const& -> detail::unwrap_optional_t<decltype(this->map(std::forward<Func>(f)))>
        {
            return map(std::forward<Func>(f)).unwrap();
        }

        /// \returns The same as `map(std::forward<Func>(f)).unwrap()`.
        /// \notes This is useful for functions that return an optional type itself,
        /// the optional will be "flattened" properly.
        template <typename Func>
        auto bind(
            Func&& f) && -> detail::unwrap_optional_t<decltype(this->map(std::forward<Func>(f)))>
        {
            return map(std::forward<Func>(f)).unwrap();
        }

    private:
        template <typename T>
        using remove_cv_ref =
            typename std::remove_cv<typename std::remove_reference<T>::type>::type;

    public:
        /// \returns If the optional is not empty, `std::forward<Func>(f)(value())` converted to the type `T` without cv or references.
        /// Otherwise returns `std::forward<T>(t)`.
        /// \requires `f` must be callable with `const value_type&`.
        /// \notes This is similar to `map()` but does not wrap the resulting type in an optional.
        /// Hence a fallback value must be provided.
        template <typename T, typename Func>
        auto transform(T&& t, Func&& f) const& -> remove_cv_ref<T>
        {
            if (has_value())
                return static_cast<remove_cv_ref<T>>(std::forward<Func>(f)(value()));
            return std::forward<T>(t);
        }

        /// \returns If the optional is not empty, `std::forward<Func>(f)(std::move(value()))` converted to the type `T` without cv or references.
        /// Otherwise returns `std::forward<T>(t)`.
        /// \requires `f` must be callable with `value_type&&`.
        /// \notes This is similar to `map()` but does not wrap the resulting type in an optional.
        /// Hence a fallback value must be provided.
        template <typename T, typename Func>
        auto transform(T&& t, Func&& f) && -> remove_cv_ref<T>
        {
            if (has_value())
                return static_cast<remove_cv_ref<T>>(std::forward<Func>(f)(std::move(value())));
            return std::forward<T>(t);
        }

        /// \returns A `basic_optional` with the value of `std::forward<Func>(f)(*this)`.
        /// If the result of `f` is a `basic_optional` itself, it will be returned instead,
        /// i.e. it calls `unwrap()`.
        /// \requires `f` must be callable with `decltype(*this)`,
        /// i.e. a `const` lvalue reference to the type of this optional.
        template <typename Func>
        auto then(Func&& f)
            const& -> detail::unwrap_optional_t<rebind<decltype(std::forward<Func>(f)(*this))>>
        {
            using result_type = decltype(std::forward<Func>(f)(*this));
            return rebind<result_type>(std::forward<Func>(f)(*this)).unwrap();
        }

        /// \returns A `basic_optional` with the value of `std::forward<Func>(f)(std::move(*this))`.
        /// If the result of `f` is a `basic_optional` itself, it will be returned instead,
        /// i.e. it calls `unwrap()`.
        /// \requires `f` must be callable with `decltype(std::move(*this))`,
        /// i.e. an rvalue reference to the type of this optional.
        template <typename Func>
        auto then(Func&& f) && -> detail::
            unwrap_optional_t<rebind<decltype(std::forward<Func>(f)(std::move(*this)))>>
        {
            using result_type = decltype(std::forward<Func>(f)(std::move(*this)));
            return rebind<result_type>(std::forward<Func>(f)(std::move(*this))).unwrap();
        }
    };

    /// \effects Calls the `operator()` of `f` passing it the value of `opt`,
    /// if it has a value.
    /// Otherwise does nothing.
    /// \notes An `Optional` here is every type with functions named `has_value()` and `value()`.
    template <class Optional, typename Func>
    void with(Optional&& opt, Func&& f)
    {
        if (opt.has_value())
            std::forward<Func>(f)(std::forward<Optional>(opt).value());
    }

    /// \exclude
    namespace detail
    {
        template <typename A, typename B>
        struct common_type
        {
            using type = typename std::common_type<A, B>::type;
        };

        template <typename A>
        struct common_type<A, void>
        {
            using type = typename std::common_type<A>::type;
        };

        template <typename A>
        struct common_type<void, A>
        {
            using type = typename std::common_type<A>::type;
        };

        template <>
        struct common_type<void, void>
        {
            using type = void;
        };

        template <bool Save, typename Visitor, typename... Optional>
        struct visit_optional;

        template <bool Save, typename Visitor, typename Optional>
        struct visit_optional<Save, Visitor, Optional>
        {
            template <typename... Args>
            static auto call_visitor(int, Visitor&& visitor, Args&&... args)
                -> decltype(std::forward<Visitor>(visitor)(std::forward<Args>(args)...))
            {
                return std::forward<Visitor>(visitor)(std::forward<Args>(args)...);
            }

            template <typename... Args>
            static void call_visitor(short, Visitor&&, Args&&...)
            {
                static_assert(!Save, "call to visitor does not cover all possible combinations");
            }

            template <typename... Args>
            static auto call(Visitor&& visitor, Optional&& opt, Args&&... args) ->
                typename common_type<decltype(call_visitor(0, std::forward<Visitor>(visitor),
                                                           std::forward<Args>(args)..., nullopt)),
                                     decltype(
                                         call_visitor(0, std::forward<Visitor>(visitor),
                                                      std::forward<Args>(args)...,
                                                      std::forward<Optional>(opt).value()))>::type
            {
                return opt.has_value() ? call_visitor(0, std::forward<Visitor>(visitor),
                                                      std::forward<Args>(args)...,
                                                      std::forward<Optional>(opt).value()) :
                                         call_visitor(0, std::forward<Visitor>(visitor),
                                                      std::forward<Args>(args)..., nullopt);
            }
        };

        template <bool Save, typename Visitor, typename Optional, typename... Rest>
        struct visit_optional<Save, Visitor, Optional, Rest...>
        {
            template <typename... Args>
            static auto call(Visitor&& visitor, Optional&& opt, Rest&&... rest, Args&&... args) ->
                typename common_type<decltype(visit_optional<Save, Visitor, Rest...>::call(
                                         std::forward<Visitor>(visitor),
                                         std::forward<Rest>(rest)..., std::forward<Args>(args)...,
                                         std::forward<Optional>(opt).value())),
                                     decltype(visit_optional<Save, Visitor, Rest...>::call(
                                         std::forward<Visitor>(visitor),
                                         std::forward<Rest>(rest)..., std::forward<Args>(args)...,
                                         nullopt))>::type
            {
                return opt.has_value() ?
                           visit_optional<Save, Visitor,
                                          Rest...>::call(std::forward<Visitor>(visitor),
                                                         std::forward<Rest>(rest)...,
                                                         std::forward<Args>(args)...,
                                                         std::forward<Optional>(opt).value()) :
                           visit_optional<Save, Visitor, Rest...>::call(std::forward<Visitor>(
                                                                            visitor),
                                                                        std::forward<Rest>(rest)...,
                                                                        std::forward<Args>(args)...,
                                                                        nullopt);
            }
        };

        template <typename T>
        struct visitor_allow_incomplete
        {
            template <typename U, typename = typename U::incomplete_visitor>
            static std::true_type test(int);

            template <typename U>
            static std::true_type test(int, decltype(U::incomplete_visitor));

            template <typename U>
            static std::false_type test(short);

            static const bool value = decltype(test<typename std::decay<T>::type>(0))::value;
        };

        template <typename Visitor, class... Optionals>
        auto visit(Visitor&& visitor, Optionals&&... optionals) -> decltype(
            detail::visit_optional<!visitor_allow_incomplete<Visitor>::value,
                                   decltype(std::forward<Visitor>(visitor)),
                                   decltype(std::forward<Optionals>(
                                       optionals))...>::call(std::forward<Visitor>(visitor),
                                                             std::forward<Optionals>(optionals)...))
        {
            return detail::visit_optional<!visitor_allow_incomplete<Visitor>::value,
                                          decltype(std::forward<Visitor>(visitor)),
                                          decltype(std::forward<Optionals>(
                                              optionals))...>::call(std::forward<Visitor>(visitor),
                                                                    std::forward<Optionals>(
                                                                        optionals)...);
        }
    } // namespace detail

    /// \effects Effectively calls `visitor((optionals.has_value() ? optionals.value() : nullopt)...)`,
    /// i.e. the `operator()` of `visitor` passing it `sizeof...(Optionals)` arguments,
    /// where the `i`th argument is the `value()` of the `i`th optional or `nullopt`, if it has none.
    /// If the particular combination of types is not overloaded,
    /// the program is ill-formed,
    /// unelss the `Visitor` provides a member named `incomplete_visitor`,
    /// then `visit()` does not do anything instead of the error.
    /// \returns The result of the chosen `operator()`,
    /// it's the type is the common type of all possible combinations.
    /// \notes An `Optional` here is every type with functions named `has_value()` and `value()`.
    template <typename Visitor, class... Optionals>
    auto visit(Visitor&& visitor, Optionals&&... optionals)
        -> decltype(detail::visit(std::forward<Visitor>(visitor),
                                  std::forward<Optionals>(optionals)...))
    {
        return detail::visit(std::forward<Visitor>(visitor), std::forward<Optionals>(optionals)...);
    }

    /// \exclude
    namespace detail
    {
        template <class Optional>
        bool all_have_value(Optional& optional)
        {
            return optional.has_value();
        }

        template <class Optional, class... Rest>
        bool all_have_value(Optional& optional, Rest&... rest)
        {
            return optional.has_value() && all_have_value(rest...);
        }
    } // namespace detail

    /// \returns If all optionals have a value, returns an optional with the value `f(optionals.value()...)` (perfectly forwarded).
    /// Otherwise returns an empty optional.
    /// \notes An `Optional` here is every type with functions named `has_value()` and `value()` and default constructor.
    template <class ResultOptional, typename Func, class... Optionals>
    ResultOptional apply(Func&& f, Optionals&&... optionals)
    {
        if (detail::all_have_value(optionals...))
            return std::forward<Func>(f)(std::forward<Optionals>(optionals).value()...);
        return {};
    }

#define TYPE_SAFE_DETAIL_MAKE_OP(Op, Expr, Expr2)                                                  \
    template <class StoragePolicy>                                                                 \
    bool operator Op(const basic_optional<StoragePolicy>& lhs, nullopt_t)                          \
    {                                                                                              \
        return Expr;                                                                               \
    }                                                                                              \
    template <class StoragePolicy>                                                                 \
    bool operator Op(nullopt_t, const basic_optional<StoragePolicy>& rhs)                          \
    {                                                                                              \
        return Expr2;                                                                              \
    }

    // equal to nullopt when empty
    TYPE_SAFE_DETAIL_MAKE_OP(==, !lhs.has_value(), !rhs.has_value())
    // unequal to nullopt when has a value
    TYPE_SAFE_DETAIL_MAKE_OP(!=, lhs.has_value(), rhs.has_value())
    // nothing is less than nullopt, nullopt only less then rhs if rhs has a value
    TYPE_SAFE_DETAIL_MAKE_OP(<, false, rhs.has_value())
    // lhs <= nullopt iff lhs empty, nullopt <= rhs is always true
    TYPE_SAFE_DETAIL_MAKE_OP(<=, !lhs.has_value(), true)
    // lhs > nullopt iff lhs not empty, nullopt > rhs is always false
    TYPE_SAFE_DETAIL_MAKE_OP(>, lhs.has_value(), false)
    // lhs >= nullopt is always true, nullopt >= rhs iff rhs empty
    TYPE_SAFE_DETAIL_MAKE_OP(>=, true, !rhs.has_value())

#undef TYPE_SAFE_DETAIL_MAKE_OP

#define TYPE_SAFE_DETAIL_MAKE_OP(Op, Expr, Expr2)                                                  \
    template <class StoragePolicy>                                                                 \
    auto operator Op(const basic_optional<StoragePolicy>&      lhs,                                \
                     const typename StoragePolicy::value_type& rhs)                                \
        ->decltype(lhs.value() Op rhs)                                                             \
    {                                                                                              \
        return Expr;                                                                               \
    }                                                                                              \
    template <class StoragePolicy>                                                                 \
    auto operator Op(const typename StoragePolicy::value_type& lhs,                                \
                     const basic_optional<StoragePolicy>&      rhs)                                \
        ->decltype(lhs Op rhs.value())                                                             \
    {                                                                                              \
        return Expr2;                                                                              \
    }

    // equal iff optional has value and value matches
    TYPE_SAFE_DETAIL_MAKE_OP(==, lhs.has_value() && lhs.value() == rhs,
                             rhs.has_value() && lhs == rhs.value())
    // unequal if optional does not have value or value does not match
    TYPE_SAFE_DETAIL_MAKE_OP(!=, !lhs.has_value() || lhs.value() != rhs,
                             !rhs.has_value() || lhs != rhs.value())
    // opt < value if opt empty or opt.value() < value
    // value < opt iff opt not empty and value < opt.value()
    TYPE_SAFE_DETAIL_MAKE_OP(<, !lhs.has_value() || lhs.value() < rhs,
                             rhs.has_value() && lhs < rhs.value())
    // same as above but with <=
    TYPE_SAFE_DETAIL_MAKE_OP(<=, !lhs.has_value() || lhs.value() <= rhs,
                             rhs.has_value() && lhs <= rhs.value())
    // opt > value iff opt not empty and opt.value() > value
    // value > opt if opt empty or value > opt.value()
    TYPE_SAFE_DETAIL_MAKE_OP(>, lhs.has_value() && lhs.value() > rhs,
                             !rhs.has_value() || lhs > rhs.value())
    // same as above but with >=
    TYPE_SAFE_DETAIL_MAKE_OP(>=, lhs.has_value() && lhs.value() >= rhs,
                             !rhs.has_value() || lhs >= rhs.value())

#undef TYPE_SAFE_DETAIL_MAKE_OP

#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <class StoragePolicy>                                                                 \
    auto operator Op(const basic_optional<StoragePolicy>& lhs,                                     \
                     const basic_optional<StoragePolicy>& rhs)                                     \
        ->decltype(lhs.value() Op rhs.value())                                                     \
    {                                                                                              \
        return lhs.has_value() ? lhs.value() Op rhs : nullopt Op rhs;                              \
    }

    TYPE_SAFE_DETAIL_MAKE_OP(==)
    TYPE_SAFE_DETAIL_MAKE_OP(!=)
    TYPE_SAFE_DETAIL_MAKE_OP(<)
    TYPE_SAFE_DETAIL_MAKE_OP(<=)
    TYPE_SAFE_DETAIL_MAKE_OP(>)
    TYPE_SAFE_DETAIL_MAKE_OP(>=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

    //=== optional ===//
    /// A `StoragePolicy` for [type_safe::basic_optional<StoragePolicy>]() that is similar to [std::optional<T>]()'s implementation.
    /// It uses [std::aligned_storage]() and a `bool` flag whether a value was created.
    /// \requires `T` must not be a reference.
    template <typename T>
    class direct_optional_storage
    {
        static_assert(!std::is_reference<T>::value,
                      "T must not be a reference; use optional_ref<T> for that");

    public:
        using value_type             = typename std::remove_cv<T>::type;
        using lvalue_reference       = T&;
        using const_lvalue_reference = const T&;
        using rvalue_reference       = T&&;
        using const_rvalue_reference = const T&&;

        template <typename U>
        using rebind = direct_optional_storage<U>;

        /// \effects Initializes it in the state without value.
        direct_optional_storage() noexcept : empty_(true)
        {
        }

        /// \effects Calls the constructor of `value_type` by perfectly forwarding `args`.
        /// Afterwards `has_value()` will return `true`.
        /// \throws Anything thrown by the constructor of `value_type` in which case `has_value()` is still `false`.
        /// \requires `has_value() == false`.
        /// \notes This function does not participate in overload resolution unless `value_type` is constructible from `args`.
        template <typename... Args>
        auto create_value(Args&&... args) ->
            typename std::enable_if<std::is_constructible<value_type, Args&&...>::value>::type
        {
            ::new (as_void()) value_type(std::forward<Args>(args)...);
            empty_ = false;
        }

        /// \effects Calls the destructor of `value_type`.
        /// Afterwards `has_value()` will return `false`.
        /// \requires `has_value() == true`.
        void destroy_value() noexcept
        {
            get_value().~value_type();
            empty_ = true;
        }

        /// \returns Whether or not there is a value stored.
        bool has_value() const noexcept
        {
            return !empty_;
        }

        /// \returns A reference to the stored value.
        /// \requires `has_value() == true`.
        lvalue_reference get_value() & noexcept
        {
            return *static_cast<value_type*>(as_void());
        }

        /// \returns A `const` reference to the stored value.
        /// \requires `has_value() == true`.
        const_lvalue_reference get_value() const& noexcept
        {
            return *static_cast<const value_type*>(as_void());
        }

        /// \returns A reference to the stored value.
        /// \requires `has_value() == true`.
        rvalue_reference get_value() && noexcept
        {
            return std::move(*static_cast<value_type*>(as_void()));
        }

        /// \returns A `const` reference to the stored value.
        /// \requires `has_value() == true`.
        const_rvalue_reference get_value() const&& noexcept
        {
            return std::move(*static_cast<const value_type*>(as_void()));
        }

        /// \returns Either `get_value()` or `u` converted to `value_type`.
        /// \requires `value_type` must be copy constructible and `u` convertible to `value_type`.
        template <typename U,
                  typename =
                      typename std::enable_if<std::is_copy_constructible<value_type>::value
                                              && std::is_convertible<U&&, value_type>::value>::type>
        value_type get_value_or(U&& u) const&
        {
            return has_value() ? get_value() : static_cast<value_type>(std::forward<U>(u));
        }

        /// \returns Either `std::move(get_value())` or `u` converted to `value_type`.
        /// \requires `value_type` must be move constructible and `u` convertible to `value_type`.
        template <typename U,
                  typename =
                      typename std::enable_if<std::is_move_constructible<value_type>::value
                                              && std::is_convertible<U&&, value_type>::value>::type>
        value_type get_value_or(U&& u) &&
        {
            return has_value() ? std::move(get_value()) :
                                 static_cast<value_type>(std::forward<U>(u));
        }

    private:
        void* as_void() noexcept
        {
            return static_cast<void*>(&storage_);
        }

        const void* as_void() const noexcept
        {
            return static_cast<const void*>(&storage_);
        }

        using storage_t =
            typename std::aligned_storage<sizeof(value_type), alignof(value_type)>::type;
        storage_t storage_;
        bool      empty_;
    };

    /// A [type_safe::basic_optional<StoragePolicy>]() that use [type_safe::direct_optional_storage<T>]().
    template <typename T>
    using optional = basic_optional<direct_optional_storage<T>>;

    /// \returns A new [type_safe::optional<T>]() storing a copy of `t`.
    template <typename T>
    optional<typename std::decay<T>::type> make_optional(T&& t)
    {
        return std::forward<T>(t);
    }

    /// \returns A new [type_safe::optional<T>]() with a value created by perfectly forwarding `args` to the constructor.
    template <typename T, typename... Args>
    optional<T> make_optional(Args&&... args)
    {
        optional<T> result;
        result.emplace(std::forward<Args>(args)...);
        return result;
    }

    //=== optional reference ===//
    /// A `StoragePolicy` for [type_safe::basic_optional<StoragePolicy>]() that allows optional references.
    ///
    /// The actual `value_type` passed to the optional is [std::reference_wrapper<T>](),
    /// but the reference types are normal references, so `value()` will return a `T&`
    /// and `value_or()` takes a fallback reference of the same type and returns one of them.
    /// Assigning an optional will always change the target of the reference.
    /// You cannot pass rvalues.
    ///
    /// Depending on the const-ness of `T` is the reference to `const` or non-const as well.
    template <typename T>
    class reference_optional_storage
    {
        static_assert(!std::is_reference<T>::value, "pass the type without reference");

        struct prevent_rvalues
        {
        };

    public:
        using value_type             = std::reference_wrapper<T>;
        using lvalue_reference       = T&;
        using const_lvalue_reference = T&;
        using rvalue_reference       = prevent_rvalues;
        using const_rvalue_reference = prevent_rvalues;

        template <typename U>
        using rebind = reference_optional_storage<U>;

        /// \effects Creates it without a bound reference.
        reference_optional_storage() noexcept : pointer_(nullptr)
        {
        }

        /// \effects Binds the reference to `obj`.
        void create_value(lvalue_reference obj) noexcept
        {
            pointer_ = &obj;
        }

        /// \effects Binds the same target as `const_ref`.
        template <typename U,
                  typename = typename std::
                      enable_if<std::is_same<U, typename std::remove_const<T>::type>::value>::type>
        void create_value(const basic_optional<reference_optional_storage<U>>& const_ref)
        {
            pointer_ = const_ref.has_value() ? &const_ref.value() : nullptr;
        }

        /// \effects Same as `destroy_value()`.
        void create_value(std::nullptr_t) noexcept
        {
            destroy_value();
        }

        void create_value(T&&) = delete;

        /// \effects Unbinds the reference.
        void destroy_value() noexcept
        {
            pointer_ = nullptr;
        }

        /// \returns `true` if the reference is bound, `false` otherwise.
        bool has_value() const noexcept
        {
            return pointer_ != nullptr;
        }

        /// \returns The target of the reference.
        lvalue_reference get_value() const noexcept
        {
            return *pointer_;
        }

        /// \returns Either `get_value()` or `other`.
        lvalue_reference get_value_or(lvalue_reference other) const
        {
            return has_value() ? get_value() : other;
        }

    private:
        T* pointer_;
    };

    /// A [type_safe::basic_optional<StoragePolicy>]() that use [type_safe::reference_optional_storage<T>]().
    /// It is an optional reference.
    /// \notes `T` is the type without the reference, i.e. `optional_ref<int>`.
    template <typename T>
    using optional_ref = basic_optional<reference_optional_storage<T>>;

    /// \returns A [type_safe::optional_ref<T>]() to the pointee of `ptr` or `nullopt`.
    template <typename T>
    optional_ref<T> ref(T* ptr) noexcept
    {
        return ptr ? optional_ref<T>(*ptr) : nullopt;
    }

    /// \returns A [type_safe::optional_ref<T>]() to `const` to the pointee of `ptr` or `nullopt`.
    template <typename T>
    optional_ref<const T> cref(const T* ptr) noexcept
    {
        return ptr ? optional_ref<const T>(*ptr) : nullopt;
    }

    /// \returns A [type_safe::optional<T>]() containing a copy of the value of `ref`
    /// if there is any value.
    /// \requires `T` must be copyable.
    template <typename T>
    optional<typename std::remove_const<T>::type> copy(const optional_ref<T>& ref)
    {
        return ref.has_value() ? make_optional(ref.value()) : nullopt;
    }

    /// \returns A [type_safe::optional<T>]() containing a copy of the value of `ref` created by move constructing
    /// if ther is any value.
    /// \requires `T` must be moveable and `ref` must not be a reference to `const`.
    template <typename T>
    optional<T> move(const optional_ref<T>& ref) noexcept(
        std::is_nothrow_move_constructible<T>::value)
    {
        static_assert(!std::is_const<T>::value, "move() cannot be called on reference to const");
        return ref.has_value() ? make_optional(std::move(ref.value())) : nullopt;
    }
} // namespace type_safe

namespace std
{
    template <typename T>
    class hash;

    template <class StoragePolicy>
    class hash<type_safe::basic_optional<StoragePolicy>>
    {
        using value_type = typename StoragePolicy::value_type;
        using value_hash = std::hash<value_type>;

    public:
        std::size_t operator()(const type_safe::basic_optional<StoragePolicy>& opt) const
            noexcept(noexcept(value_hash{}(std::declval<value_type>())))
        {
            return opt ? value_hash{}(opt.value()) : 19937; // magic value
        }
    };
} // namespace std

#endif // TYPE_SAFE_OPTIONAL_HPP_INCLUDED
