// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_CONSTRAINED_TYPE_HPP_INCLUDED
#define TYPE_SAFE_CONSTRAINED_TYPE_HPP_INCLUDED

#include <type_traits>
#include <utility>

#include <type_safe/detail/assert.hpp>
#include <type_safe/detail/is_nothrow_swappable.hpp>
#include <type_safe/config.hpp>

namespace type_safe
{
    /// A `Verifier` for [type_safe::constrained_type<T, Constraint, Verifier]() that `DEBUG_ASSERT`s the constraint.
    struct assertion_verifier
    {
        template <typename Value, typename Predicate>
        static void verify(const Value& val, const Predicate& p)
        {
            DEBUG_ASSERT(p(val), detail::assert_handler{}, "value does not fulfill constraint");
        }
    };

    /// \exclude
    namespace detail
    {
        template <class Constraint, typename T>
        auto verify_static_constrained(int) -> typename Constraint::template is_valid<T>;

        template <class Constraint, typename T>
        auto verify_static_constrained(char) -> std::true_type;

        template <class Constraint, typename T>
        struct is_valid : decltype(verify_static_constrained<Constraint, T>(0))
        {
        };
    } // namespace detail

    /// A value of type `T` that always fulfills the predicate `Constraint`.
    /// The `Constraint` is checked by the `Verifier`.
    /// The `Constraint` can also provide a nested template `is_valid<T>` to statically check types.
    /// Those will be checked regardless of the `Verifier`.
    /// \requires `T` must not be a reference, `Constraint` must be a functor of type `bool(const T&)`
    /// and `Verifier` must provide a `static` function `void verify([const] T&, const Predicate&)`.
    template <typename T, typename Constraint, typename Verifier = assertion_verifier>
    class constrained_type : Constraint
    {
        static_assert(!std::is_reference<T>::value, "T must not be a reference");

    public:
        using value_type           = typename std::remove_cv<T>::type;
        using constraint_predicate = Constraint;

        /// \effects Creates it giving it a valid `value` and a `predicate`.
        /// The `value` will be copied and verified.
        /// \throws Anything thrown by the copy constructor of `value_type`.
        explicit constrained_type(const value_type& value, constraint_predicate predicate = {})
        : Constraint(std::move(predicate)), value_(value)
        {
            verify();
        }

        /// \effects Creates it giving it a valid `value` and a `predicate`.
        /// The `value` will be moved and verified.
        /// \throws Anything thrown by the the move constructor of `value_type`.
        explicit constrained_type(value_type&& value, constraint_predicate predicate = {}) noexcept(
            std::is_nothrow_constructible<value_type>::value &&
            noexcept(std::declval<constrained_type>().verify()))
        : Constraint(std::move(predicate)), value_(std::move(value))
        {
            verify();
        }

        template <typename U,
                  typename = typename std::enable_if<!detail::is_valid<constraint_predicate,
                                                                       U>::value>::type>
        constrained_type(U) = delete;

        /// \effects Copies the value and predicate of `other`.
        /// \throws Anything thrown by the copy constructor of `value_type`.
        constrained_type(const constrained_type& other) : Constraint(other), value_(other.value_)
        {
            debug_verify();
        }

        /// \effects Destroys the value.
        ~constrained_type() noexcept = default;

        /// \effects Copy assigns the stored value to the valid value `other`.
        /// It will also verify the new value prior to assigning.
        /// \throws Anything thrown by the copy assignment operator of `value_type`.
        constrained_type& operator=(const value_type& other)
        {
            Verifier::verify(other, get_constraint());
            value_ = other;
            return *this;
        }

        /// \effects Move assigns the stored value to the valid value `other`.
        /// It will also verify the new value prior to assigning.
        /// \throws Anything thrown by the move assignment operator of `value_type`.
        constrained_type& operator=(value_type&& other) noexcept(
            std::is_nothrow_move_assignable<value_type>::value &&
            noexcept(std::declval<constrained_type>().verify()))
        {
            Verifier::verify(other, get_constraint());
            value_ = std::move(other);
            return *this;
        }

        template <typename U,
                  typename = typename std::enable_if<!detail::is_valid<constraint_predicate,
                                                                       U>::value>::type>
        constrained_type& operator=(U) = delete;

        /// \effects Copies the value and predicate from `other`.
        /// \throws Anything thrown by the copy assignment operator of `value_type`.
        constrained_type& operator=(const constrained_type& other)
        {
            constrained_type tmp(other);
            swap(*this, tmp);
            return *this;
        }

        /// \effects Swaps the value and predicate of a `a` and `b`.
        /// \throws Anything thrown by the swap function of `value_type`.
        friend void swap(constrained_type& a, constrained_type& b) noexcept(
            detail::is_nothrow_swappable<value_type>::value)
        {
            a.debug_verify();
            b.debug_verify();

            using std::swap;
            swap(a.value_, b.value_);
            swap(static_cast<Constraint&>(a), static_cast<Constraint&>(b));
        }

        /// A proxy class to provide write access to the stored value.
        /// The destructor will verify the value again.
        class modifier
        {
        public:
            /// \effects Move constructs it.
            /// `other` will not verify any value afterwards.
            modifier(modifier&& other) noexcept : value_(other.value_)
            {
                other.value_ = nullptr;
            }

            /// \effects Verifies the value, if there is any.
            ~modifier() noexcept(false)
            {
                if (value_)
                    value_->verify();
            }

            /// \effects Move assigns it.
            /// `other` will not verify any value afterwards.
            modifier& operator=(modifier&& other) noexcept
            {
                value_       = other.value_;
                other.value_ = nullptr;
                return *this;
            }

            /// \returns A reference to the stored value.
            /// \requires It must not be in the moved-from state.
            value_type& get() noexcept
            {
                DEBUG_ASSERT(value_, detail::assert_handler{});
                return value_->value_;
            }

        private:
            modifier(constrained_type& value) noexcept : value_(&value)
            {
            }

            constrained_type* value_;
            friend constrained_type;
        };

        /// \returns A proxy object to provide verified write-access to the stored value.
        modifier modify() noexcept
        {
            debug_verify();
            return modifier(*this);
        }

        /// \effects Moves the stored value out of the `constrained_type`,
        /// it will not be checked further.
        /// \returns An rvalue reference to the stored value.
        /// \notes After this function is called, the object must not be used anymore
        /// except as target for assignment or in the destructor.
        value_type&& release() TYPE_SAFE_RVALUE_REF noexcept
        {
            debug_verify();
            return std::move(value_);
        }

        /// \returns A `const` reference to the stored value.
        /// \requires Any `const` operations on the `value_type` must not affect the validity of the value.
        const value_type& get_value() const noexcept
        {
            debug_verify();
            return value_;
        }

        /// \returns The predicate that determines validity.
        const constraint_predicate& get_constraint() const noexcept
        {
            return *this;
        }

    private:
        void verify() const noexcept(noexcept(
            Verifier::verify(std::declval<value_type>(),std::declval<constraint_predicate>())))
        {
            Verifier::verify(value_, get_constraint());
        }

        void debug_verify() const noexcept
        {
#if TYPE_SAFE_ENABLE_ASSERTIONS
            verify();
#endif
        }

        value_type value_;
    };

    /// \returns A [type_safe::constrained_type<T, Constraint, Verifier>]() with the given `value` and `Constraint`.
    template <typename T, typename Constraint>
    auto constrain(T&& value, Constraint c)
        -> constrained_type<typename std::decay<T>::type, Constraint>
    {
        return constrained_type<typename std::decay<T>::type, Constraint>(std::forward<T>(value),
                                                                          std::move(c));
    }

    /// \returns A [type_safe::constrained_type<T, Constraint, Verifier>]() with the given `value`,  `Constraint` and `Verifier`.
    template <class Verifier, typename T, typename Constraint>
    auto constrain(T&& value, Constraint c)
        -> constrained_type<typename std::decay<T>::type, Constraint, Verifier>
    {
        return constrained_type<typename std::decay<T>::type, Constraint, Verifier>(std::forward<T>(
                                                                                        value),
                                                                                    std::move(c));
    }

    /// \effects Calls `f` with a non-`const` reference to the stored value of the [type_safe::constrained_type<T, Constraint, Verifier>]().
    /// It checks that `f` does not change the validity of the object.
    /// \notes The same behavior can be accomplished by using the `modify()` member function.
    template <typename T, typename Constraint, class Verifier, typename Func>
    void with(constrained_type<T, Constraint, Verifier>& value, Func&& f)
    {
        auto modifier = value.modify();
        std::forward<Func>(f)(modifier.get());
    }

    /// A `Verifier` for [type_safe::constrained_type<T, Constraint, Verifier]() that doesn't check the constraint.
    struct null_verifier
    {
        template <typename Value, typename Predicate>
        static void verify(const Value&, const Predicate&)
        {
        }
    };

    /// An alias for [type_safe::constrained_type<T, Constraint, Verifier>]() that never checks the constraint.
    /// It is useful for creating tagged types:
    /// The `Constraint` - which does not need to be a predicate anymore - is a "tag" to differentiate a type in different states.
    /// For example, you could have a "sanitized" value and a "non-sanitized" value
    /// that have different types, so you cannot accidentally mix them.
    /// \notes It is only intended if the `Constrained` cannot be formalized easily and/or is expensive.
    /// Otherwise [type_safe::constrained_type<T, Constrained, Verifier>]() is recommended
    /// as it does additional runtime checks in debug mode.
    template <typename T, typename Constraint>
    using tagged_type = constrained_type<T, Constraint, null_verifier>;

    /// \returns A [type_safe::tagged_type<T, Constraint>]() with the given `value` and `Constraint`.
    template <typename T, typename Constraint>
    auto tag(T&& value, Constraint c) -> tagged_type<typename std::decay<T>::type, Constraint>
    {
        return tagged_type<typename std::decay<T>::type, Constraint>(std::forward<T>(value),
                                                                     std::move(c));
    }

    namespace constraints
    {
        /// A `Constraint` for the [type_safe::constrained_type<T, Constraint, Verifier>]().
        /// A value of a pointer type is valid if it is not equal to `nullptr`.
        /// This is borrowed from GSL's [non_null](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#a-namess-viewsagslview-views).
        struct non_null
        {
            template <typename T>
            struct is_valid : std::true_type
            {
            };

            template <typename T>
            bool operator()(const T& ptr) const noexcept
            {
                return ptr != nullptr;
            }
        };

        template <>
        struct non_null::is_valid<std::nullptr_t> : std::false_type
        {
        };

        /// A `Constraint` for the [type_safe::constrained_type<T, Constraint, Verifier>]().
        /// A value of a container type is valid if it is not empty.
        /// Empty-ness is determined with either a member or non-member function.
        class non_empty
        {
            template <typename T>
            auto is_empty(int, const T& t) const noexcept(noexcept(t.empty()))
                -> decltype(t.empty())
            {
                return !t.empty();
            }

            template <typename T>
            bool is_empty(short, const T& t) const
            {
                return !empty(t);
            }

        public:
            template <typename T>
            bool operator()(const T& t) const
            {
                return is_empty(0, t);
            }
        };

        /// A `Constraint` for the [type_safe::constrained_type<T, Constraint, Verifier>]().
        /// A value is valid if it not equal to the default constructed value.
        struct non_default
        {
            template <typename T>
            bool operator()(const T& t) const noexcept(noexcept(t == T()))
            {
                return !(t == T());
            }
        };

        /// A `Constraint` for the [type_safe::constrained_type<T, Constraint, Verifier>]().
        /// A value of a pointer-like type is valid if the expression `!value` is `false`.
        struct non_invalid
        {
            template <typename T>
            bool operator()(const T& t) const noexcept(noexcept(!!t))
            {
                return !!t;
            }
        };

        /// A `Constraint` for the [type_safe::tagged_type<T, Constraint>]().
        /// It marks an owning pointer.
        /// It is borrowed from GSL's [non_null](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#a-namess-viewsagslview-views).
        /// \notes This is not actually a predicate.
        struct owner
        {
        };
    } // namespace constraints
} // namespace type_safe

#endif // TYPE_SAFE_CONSTRAINED_TYPE_HPP_INCLUDED
