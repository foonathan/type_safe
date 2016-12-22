// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_VARIANT_HPP_INCLUDED
#define TYPE_SAFE_VARIANT_HPP_INCLUDED

#include <type_safe/detail/all_of.hpp>
#include <type_safe/detail/assign_or_construct.hpp>
#include <type_safe/detail/common_type.hpp>
#include <type_safe/detail/copy_move_control.hpp>
#include <type_safe/detail/is_nothrow_swappable.hpp>
#include <type_safe/optional_ref.hpp>
#include <type_safe/tagged_union.hpp>

namespace type_safe
{
    /// \exclude
    namespace detail
    {
        template <typename... Types>
        struct traits_impl
        {
            using copy_constructible = all_of<std::is_copy_constructible<Types>::value...>;
            using move_constructible = all_of<std::is_move_constructible<Types>::value...>;
            using nothrow_move_constructible =
                all_of<std::is_nothrow_move_constructible<Types>::value...>;
            using nothrow_move_assignable =
                all_of<nothrow_move_constructible::value,
                       (std::is_move_assignable<Types>::value ?
                            std::is_nothrow_move_assignable<Types>::value :
                            true)...>;
            using nothrow_swappable =
                all_of<nothrow_move_constructible::value, is_nothrow_swappable<Types>::value...>;
        };

        template <typename... Types>
        using traits = traits_impl<typename std::decay<Types>::type...>;

        template <class VariantPolicy, class Union>
        class copy_assign_union_value;

        template <class VariantPolicy, typename... Types>
        class copy_assign_union_value<VariantPolicy, tagged_union<Types...>>
        {
        public:
            static void assign(tagged_union<Types...>& dest, const tagged_union<Types...>& org)
            {
                auto idx = static_cast<std::size_t>(org.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), detail::assert_handler{});
                callbacks[idx](dest, org);
            }

        private:
            template <typename T,
                      typename = typename std::enable_if<std::is_copy_assignable<T>::value>::type>
            static void do_assign(union_type<T> type, tagged_union<Types...>& dest,
                                  const tagged_union<Types...>& org)
            {
                dest.value(type) = org.value(type);
            }

            template <typename T,
                      typename std::enable_if<!std::is_copy_assignable<T>::value, int>::type = 0>
            static void do_assign(union_type<T> type, tagged_union<Types...>& dest,
                                  const tagged_union<Types...>& org)
            {
                VariantPolicy::change_value(type, dest, org.value(type));
            }

            template <typename T>
            static void assign_impl(tagged_union<Types...>& dest, const tagged_union<Types...>& org)
            {
                constexpr auto id = tagged_union<Types...>::type_id(union_type<T>{});
                DEBUG_ASSERT(org.type() == id, detail::assert_handler{});

                if (dest.type() == id)
                    do_assign(union_type<T>{}, dest, org);
                else
                    VariantPolicy::change_value(union_type<T>{}, dest, org.value(union_type<T>{}));
            }

            using callback_type = void (*)(tagged_union<Types...>&, const tagged_union<Types...>&);
            static constexpr callback_type callbacks[] = {&assign_impl<Types>...};
        };
        template <class VariantPolicy, typename... Types>
        constexpr
            typename copy_assign_union_value<VariantPolicy, tagged_union<Types...>>::callback_type
                copy_assign_union_value<VariantPolicy, tagged_union<Types...>>::callbacks[];

        template <class VariantPolicy, class Union>
        class move_assign_union_value;

        template <class VariantPolicy, typename... Types>
        class move_assign_union_value<VariantPolicy, tagged_union<Types...>>
        {
        public:
            static void assign(tagged_union<Types...>& dest, tagged_union<Types...>&& org)
            {
                auto idx = static_cast<std::size_t>(org.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), detail::assert_handler{});
                callbacks[idx](dest, std::move(org));
            }

        private:
            template <typename T,
                      typename = typename std::enable_if<std::is_move_assignable<T>::value>::type>
            static void do_assign(union_type<T> type, tagged_union<Types...>& dest,
                                  tagged_union<Types...>&& org)
            {
                dest.value(type) = std::move(org).value(type);
            }

            template <typename T,
                      typename std::enable_if<!std::is_move_assignable<T>::value, int>::type = 0>
            static void do_assign(union_type<T> type, tagged_union<Types...>& dest,
                                  tagged_union<Types...>&& org)
            {
                VariantPolicy::change_value(type, dest, std::move(org).value(type));
            }

            template <typename T>
            static void assign_impl(tagged_union<Types...>& dest, tagged_union<Types...>&& org)
            {
                constexpr auto id = tagged_union<Types...>::type_id(union_type<T>{});
                DEBUG_ASSERT(org.type() == id, detail::assert_handler{});

                if (dest.type() == id)
                    do_assign(union_type<T>{}, dest, std::move(org));
                else
                    VariantPolicy::change_value(union_type<T>{}, dest,
                                                std::move(org).value(union_type<T>{}));
            }

            using callback_type = void (*)(tagged_union<Types...>&, tagged_union<Types...>&&);
            static constexpr callback_type callbacks[] = {&assign_impl<Types>...};
        };
        template <class VariantPolicy, typename... Types>
        constexpr
            typename move_assign_union_value<VariantPolicy, tagged_union<Types...>>::callback_type
                move_assign_union_value<VariantPolicy, tagged_union<Types...>>::callbacks[];

        template <class VariantPolicy, class Union>
        class swap_union;

        template <class VariantPolicy, typename... Types>
        class swap_union<VariantPolicy, tagged_union<Types...>>
        {
        public:
            static void swap(tagged_union<Types...>& a, tagged_union<Types...>& b)
            {
                auto idx = static_cast<std::size_t>(a.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), detail::assert_handler{});
                callbacks[idx](a, b);
            }

        private:
            template <typename T>
            static void swap_impl(tagged_union<Types...>& a, tagged_union<Types...>& b)
            {
                constexpr auto id = tagged_union<Types...>::type_id(union_type<T>{});
                DEBUG_ASSERT(a.type() == id, detail::assert_handler{});

                if (b.type() == id)
                {
                    using std::swap;
                    swap(a.value(union_type<T>{}), b.value(union_type<T>{}));
                }
                else
                {
                    T tmp(std::move(a).value(union_type<T>{})); // save old value from a
                    // assign a to value in b
                    move_assign_union_value<VariantPolicy, Types...>::assign(a, std::move(b));
                    // change value in b to tmp
                    VariantPolicy::change_value(union_type<T>{}, b, std::move(tmp));
                }
            }

            using callback_type = void (*)(tagged_union<Types...>&, tagged_union<Types...>&);
            static constexpr callback_type callbacks[] = {&swap_impl<Types>...};
        };
        template <class VariantPolicy, typename... Types>
        constexpr typename swap_union<VariantPolicy, tagged_union<Types...>>::callback_type
            swap_union<VariantPolicy, tagged_union<Types...>>::callbacks[];

        template <typename Functor, class Union>
        class map_union;

        template <typename Functor, typename... Types>
        class map_union<Functor, tagged_union<Types...>>
        {
        public:
            static void map(tagged_union<Types...>& res, const tagged_union<Types...>& tunion,
                            Functor&& f)
            {
                DEBUG_ASSERT(!res.has_value(), assert_handler{});
                auto idx = static_cast<std::size_t>(tunion.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), assert_handler{});
                copy_callbacks[idx](res, tunion, std::forward<Functor>(f));
            }
            static void map(tagged_union<Types...>& res, tagged_union<Types...>&& tunion,
                            Functor&& f)
            {
                DEBUG_ASSERT(!res.has_value(), assert_handler{});
                auto idx = static_cast<std::size_t>(tunion.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), assert_handler{});
                move_callbacks[idx](res, std::move(tunion), std::forward<Functor>(f));
            }

        private:
            template <typename T, typename Result =
                                      decltype(std::declval<Functor&&>()(std::declval<const T&>()))>
            static void call(int, tagged_union<Types...>& res, const tagged_union<Types...>& tunion,
                             Functor&& f)
            {
                auto&& result = std::forward<Functor>(f)(tunion.value(union_type<T>{}));
                res.emplace(union_type<typename std::decay<Result>::type>{},
                            std::forward<Result>(result));
            }
            template <typename T,
                      typename Result = decltype(std::declval<Functor&&>()(std::declval<T&&>()))>
            static void call(int, tagged_union<Types...>& res, tagged_union<Types...>&& tunion,
                             Functor&& f)
            {
                auto&& result = std::forward<Functor>(f)(std::move(tunion).value(union_type<T>{}));
                res.emplace(union_type<typename std::decay<Result>::type>{},
                            std::forward<Result>(result));
            }

            template <typename T>
            static void call(short, tagged_union<Types...>& res,
                             const tagged_union<Types...>& tunion, Functor&&)
            {
                res.emplace(union_type<T>{}, tunion.value(union_type<T>{}));
            }
            template <typename T>
            static void call(short, tagged_union<Types...>& res, tagged_union<Types...>&& tunion,
                             Functor&&)
            {
                res.emplace(union_type<T>{}, std::move(tunion).value(union_type<T>{}));
            }

            template <typename T>
            static void map_impl_copy(tagged_union<Types...>&       res,
                                      const tagged_union<Types...>& tunion, Functor&& f)
            {
                call(0, res, tunion, std::forward<Functor>(f));
            }
            template <typename T>
            static void map_impl_move(tagged_union<Types...>& res, tagged_union<Types...>&& tunion,
                                      Functor&& f)
            {
                call(0, res, std::move(tunion), std::forward<Functor>(f));
            }

            using copy_callback_type = void (*)(tagged_union<Types...>&,
                                                const tagged_union<Types...>&, Functor&&);
            using move_callback_type = void (*)(tagged_union<Types...>&, tagged_union<Types...>&&,
                                                Functor&&);

            static constexpr copy_callback_type copy_callbacks[] = {&map_impl_copy<Types>...};
            static constexpr move_callback_type move_callbacks[] = {&map_impl_move<Types>...};
        };
        template <typename Functor, typename... Types>
        constexpr typename map_union<Functor, tagged_union<Types...>>::copy_callback_type
            map_union<Functor, tagged_union<Types...>>::copy_callbacks[];
        template <typename Functor, typename... Types>
        constexpr typename map_union<Functor, tagged_union<Types...>>::move_callback_type
            map_union<Functor, tagged_union<Types...>>::move_callbacks[];

        template <class VariantPolicy, typename... Types>
        class variant_storage
        {
            using traits = detail::traits<Types...>;

        public:
            variant_storage() noexcept = default;

            variant_storage(const variant_storage& other)
            {
                if (other.storage_.has_value())
                    copy(storage_, other.storage_);
            }

            variant_storage(variant_storage&& other) noexcept(
                traits::nothrow_move_constructible::value)
            {
                if (other.storage_.has_value())
                    move(storage_, std::move(other.storage_));
            }

            ~variant_storage() noexcept
            {
                if (storage_.has_value())
                    destroy(storage_);
            }

            variant_storage& operator=(const variant_storage& other)
            {
                if (storage_.has_value() && other.storage_.has_value())
                    copy_assign_union_value<VariantPolicy,
                                            tagged_union<Types...>>::assign(storage_,
                                                                            other.storage_);
                else if (storage_.has_value() && !other.storage_.has_value())
                    destroy(storage_);
                else if (!storage_.has_value() && other.storage_.has_value())
                    copy(storage_, other.storage_);

                return *this;
            }

            variant_storage& operator=(variant_storage&& other) noexcept(
                traits::nothrow_move_assignable::value)
            {
                if (storage_.has_value() && other.storage_.has_value())
                    copy_assign_union_value<VariantPolicy,
                                            tagged_union<Types...>>::assign(storage_,
                                                                            other.storage_);
                else if (storage_.has_value() && !other.storage_.has_value())
                    destroy(storage_);
                else if (!storage_.has_value() && other.storage_.has_value())
                    copy(storage_, other.storage_);

                return *this;
            }

            tagged_union<Types...>& get_union() noexcept
            {
                return storage_;
            }

            const tagged_union<Types...>& get_union() const noexcept
            {
                return storage_;
            }

        private:
            tagged_union<Types...> storage_;
        };

        template <typename... Types>
        using variant_copy = copy_control<traits<Types...>::copy_constructible::value>;

        template <typename... Types>
        using variant_move = move_control<traits<Types...>::move_constructible::value>;

        template <typename Type, class Union, typename T, typename... Args>
        using enable_variant_type_impl =
            typename std::enable_if<typename Union::type_id(union_type<T>{}) != Union::invalid_type
                                        && std::is_constructible<T, Args...>::value,
                                    Type>::type;

        template <typename Type, class Union, typename T, typename... Args>
        using enable_variant_type =
            enable_variant_type_impl<Type, Union, typename std::decay<T>::type, Args...>;
    } // namespace detail

    /// Convenience alias for [ts::union_type]().
    /// \module variant
    template <typename T>
    using variant_type = union_type<T>;

    /// Convenience alias for [ts::union_types]().
    /// \module variant
    template <typename... Ts>
    using variant_types = union_types<Ts...>;

    /// Tag type to mark a [ts::basic_variant]() without a value.
    /// \module variant
    struct nullvar_t
    {
        constexpr nullvar_t()
        {
        }
    };

    /// Tag object of type [ts::nullvar_t]().
    /// \module variant
    constexpr nullvar_t nullvar;

    template <class VariantPolicy, typename HeadT, typename... TailT>
    class basic_variant : detail::variant_copy<HeadT, TailT...>,
                          detail::variant_move<HeadT, TailT...>
    {
        using union_t = tagged_union<HeadT, TailT...>;
        using traits  = detail::traits<HeadT, TailT...>;

    public:
        using types   = typename union_t::types;
        using type_id = typename union_t::type_id;

        static constexpr type_id invalid_type = union_t::invalid_type;

        //=== constructors/destructors/assignment/swap ===//
        /// \effects Initializes the variant to the empty state.
        /// \notes This constructor only participates in overload resolution,
        /// if the policy allows an empty variant.
        /// \group default
        /// \param Dummy
        /// \exclude
        /// \param 1
        /// \exclude
        template <
            typename Dummy = void,
            typename = typename std::enable_if<VariantPolicy::allow_empty::value, Dummy>::type>
        basic_variant() noexcept
        {
        }

        /// \group default
        /// \param Dummy
        /// \exclude
        /// \param 1
        /// \exclude
        template <
            typename Dummy = void,
            typename = typename std::enable_if<VariantPolicy::allow_empty::value, Dummy>::type>
        basic_variant(nullvar_t) noexcept : basic_variant()
        {
        }

        /// Copy (1)/move (2) constructs a variant.
        /// \effects If the other variant is not empty, it will call [ts::copy](standardese://ts::copy_union/) (1)
        /// or [ts::move](standardese://ts::move_union/) (2).
        /// \throws Anything thrown by the copy (1)/move (2) constructor.
        /// \notes This constructor only participates in overload resolution,
        /// if all types are copy (1)/move (2) constructible./
        /// \notes The move constructor only moves the stored value,
        /// and does not make the other variant empty.
        /// \group copy_move_ctor
        basic_variant(const basic_variant&) = default;

        /// \group copy_move_ctor
        basic_variant(basic_variant&&) noexcept(traits::nothrow_move_constructible::value) =
            default;

        /// Initializes it containing a new object of the given type.
        /// \effects Creates it by calling `T`s constructor with the perfectly forwarded arguments.
        /// \throws Anything thrown by `T`s constructor.
        /// \notes This constructor does not participate in overload resolution,
        /// unless `T` is a valid type for the variant and constructible from the arguments.
        /// \signature template <typename T, typename ... Args>\nbasic_variant(variant_type<T>, Args&&... args);
        template <typename T, typename... Args>
        explicit basic_variant(
            detail::enable_variant_type<variant_type<T>, union_t, T, Args&&...> type,
            Args&&... args)
        {
            storage_.get_union().emplace(type, std::forward<Args>(args)...);
        }

        /// Initializes it with a copy of the given object.
        /// \effects Same as the type + argument constructor called with the decayed type of the argument
        /// and the object perfectly forwarded.
        /// \throws Anything thrown by `T`s copy/move constructor.
        /// \notes This constructor does not participate in overload resolution,
        /// unless `T` is a valid type for the variant and copy/move constructible.
        /// \param 1
        /// \exclude
        template <typename T, typename = detail::enable_variant_type<void, union_t, T, T&&>>
        basic_variant(T&& obj)
        : basic_variant(variant_type<typename std::decay<T>::type>{}, std::forward<T>(obj))
        {
        }

        /// \effects Destroys the currently stored value,
        /// if there is any.
        ~basic_variant() noexcept = default;

        /// Copy (1)/move (2) assigns a variant.
        /// \effects If the other variant is empty,
        /// makes this one empty as well.
        /// Otherwise let the other variant contains an object of type `T`.
        /// If this variant contains the same type and there is a copy (1)/move (2) assignment operator available,
        /// assigns the object to this object.
        /// Else forwards to the variant policy's `change_value()` function.
        /// \throws Anything thrown by either the copy (1)/move (2) assignment operator
        /// or copy (1)/move (2) constructor.
        /// If the assignment operator throws, the variant will contain the partially assigned object.
        /// If the constructor throws, the state depends on the variant policy.
        /// \notes This function does not participate in overload resolution,
        /// unless all types are copy (1)/move (2) constructible.
        /// \group copy_move_assign
        basic_variant& operator=(const basic_variant&) = default;
        /// \signature template <typename T, typename ... Args>\nvoid emplace(variant_type<T> type, Args&&... args);
        /// \group copy_move_assign
        basic_variant& operator=(basic_variant&&) noexcept(traits::nothrow_move_assignable::value) =
            default;

        /// Swaps two variants.
        /// \effects There are four cases:
        /// * Both variants are empty. Then the function has no effect.
        /// * Both variants contain the same type, `T`. Then it calls swap on the stored type.
        /// * Both variants contain a type, but different types.
        /// Then it swaps the variant by move constructing the objects from one type to the other,
        /// using the variant policy.
        /// * Only one variant contains an object. Then it moves the value to the empty variant,
        /// and destroys it in the non-empty variant.
        ///
        /// \effects In either case, it will only call the swap() function or the move constructor.
        /// \throws Anything thrown by the swap function,
        /// in which case both variants contain the partially swapped values,
        /// or the mvoe constructor, in which case the exact behavior depends on the variant policy.
        friend void swap(basic_variant& a,
                         basic_variant& b) noexcept(traits::nothrow_swappable::value)
        {
            auto& a_union = a.storage_.get_union();
            auto& b_union = b.storage_.get_union();

            if (a_union.has_value() && !b_union.has_value())
            {
                b = std::move(a);
                a.reset();
            }
            else if (!a_union.has_value() && b_union.has_value())
            {
                a = std::move(b);
                b.reset();
            }
            else if (a_union.type() == b_union.type())
                detail::swap_union<VariantPolicy, union_t>::swap(a_union, b_union);
        }

        //=== modifiers ===//
        /// \effects Destroys the stored value in the variant, if any.
        /// \notes This function only participate in overload resolution,
        /// if the variant policy allows the empty state.
        /// \param Dummy
        /// \exclude
        /// \param 1
        /// \exclude
        template <
            typename Dummy = void,
            typename = typename std::enable_if<VariantPolicy::allow_empty::value, Dummy>::type>
        void reset() noexcept
        {
            if (storage_.get_union().has_value())
                destroy(storage_.get_union());
        }

        /// Changes the value to a new object of the given type.
        /// \effects If the variant contains an object of the same type,
        /// assigns the argument to it.
        /// Otherwise behaves as the other emplace version.
        /// \throws Anything thrown by the chosen assignment operator
        /// or the other `emplace()`.
        /// If the assignment operator throws,
        /// the variant contains a partially assigned oject.
        /// Otherwise it depends on the variant policy.
        /// \notes This function does not participate in overload resolution,
        /// unless `T` is a valid type for the variant and assignable from the argument
        /// without creating an additional temporary.
        /// \signature template <typename T, typename Arg>\nvoid emplace(variant_type<T> type, Arg&& args);
        template <
            typename T, typename Arg,
            typename = typename std::enable_if<detail::is_direct_assignable<T, Arg&&>::value>::type>
        auto emplace(variant_type<T> type, Arg&& arg)
            -> detail::enable_variant_type<void, union_t, T, Arg&&>
        {
            constexpr auto idx = union_t::type_id(type);
            if (storage_.get_union().type() == idx)
                storage_.get_union().value(type) = std::forward<Arg>(arg);
            else
                emplace_impl(type, std::forward<Arg>(arg));
        }

        /// Changes the value to a new object of given type.
        /// \effects If variant is empty, creates the object directly inplace
        /// by perfectly forwarding the arguments.
        /// Otherwise it forwards to the variant policy's `change_value()` function.
        /// \throws Anything thrown by `T`s constructor or possibly move constructor.
        /// If the variant was empty before, it is still empty afterwards.
        /// Otherwise the state depends on the policy.
        /// \notes This function does not participate in overload resolution,
        /// unless `T` is a valid type for the variant and constructible from the arguments.
        /// \signature template <typename T, typename ... Args>\nvoid emplace(variant_type<T> type, Args&&... args);
        template <typename T, typename... Args>
        auto emplace(variant_type<T> type, Args&&... args)
            -> detail::enable_variant_type<void, union_t, T, Args&&...>
        {
            emplace_impl(type, std::forward<Args>(args)...);
        }

    private:
        template <typename T, typename... Args>
        void emplace_impl(variant_type<T> type, Args&&... args)
        {
            if (storage_.get_union().has_value())
                VariantPolicy::change_value(type, storage_.get_union(),
                                            std::forward<Args>(args)...);
            else
                storage_.get_union().emplace(type, std::forward<Args>(args)...);
        }

    public:
        //=== abservers ===//
        /// \returns The type id representing the type of the value currently stored in the variant.
        /// \notes If it does not have a value stored, returns [*invalid_type]().
        type_id type() const noexcept
        {
            return storage_.get_union().type();
        }

        /// \returns `true` if the variant currently contains a value,
        /// `false` otherwise.
        /// \notes Depending on the variant policy,
        /// it can be guaranteed to return `true` all the time.
        /// \group has_value
        bool has_value() const noexcept
        {
            return storage_.get_union().has_value();
        }

        /// \group has_value
        explicit operator bool() const noexcept
        {
            return has_value();
        }

        /// \group has_value
        bool has_value(variant_type<nullvar_t>) const noexcept
        {
            return has_value();
        }

        /// \returns `true` if the variant currently stores an object of type `T`,
        /// `false` otherwise.
        /// \notes `T` must not necessarily be a type that can be stored in the variant.
        template <typename T>
        bool has_value(variant_type<T> type) const noexcept
        {
            return type() == type_id(type);
        }

        /// \returns A (`const`) lvalue (1, 2)/rvalue (3, 4) reference to the stored object of the given type.
        /// \requires The variant must currently store an object of the given type,
        /// i.e. `has_value(type)` must return `true`.
        /// \group value
        /// \param 1
        /// \exclude
        template <typename T, typename = typename std::enable_if<type_id(variant_type<T>{})
                                                                 != invalid_type>::type>
        T& value(variant_type<T> type) TYPE_SAFE_LVALUE_REF noexcept
        {
            return storage_.get_union().value(type);
        }

        /// \group value
        /// \param 1
        /// \exclude
        template <typename T, typename = typename std::enable_if<type_id(variant_type<T>{})
                                                                 != invalid_type>::type>
        const T& value(variant_type<T> type) const TYPE_SAFE_LVALUE_REF noexcept
        {
            return storage_.get_union().value(type);
        }

#if TYPE_SAFE_USE_REF_QUALIFIERS
        /// \group value
        /// \param 1
        /// \exclude
        template <typename T, typename = typename std::enable_if<type_id(variant_type<T>{})
                                                                 != invalid_type>::type>
            T&& value(variant_type<T> type) && noexcept
        {
            return std::move(storage_.get_union()).value(type);
        }

        /// \group value
        /// \param 1
        /// \exclude
        template <typename T, typename = typename std::enable_if<type_id(variant_type<T>{})
                                                                 != invalid_type>::type>
        const T&& value(variant_type<T> type) const && noexcept
        {
            return std::move(storage_.get_union()).value(type);
        }
#endif

        /// \returns A (`const`) [ts::optional_ref]() (1, 2)/[ts::optional_xvalue_ref]() to the stored value of given type.
        /// If it stores a different type, returns a null reference.
        /// \group optional_value
        template <typename T>
        optional_ref<T> optional_value(variant_type<T> type) TYPE_SAFE_LVALUE_REF noexcept
        {
            return has_value(type) ? type_safe::ref(storage_.get_union().value(type)) : nullptr;
        }

        /// \group optional_value
        template <typename T>
        optional_ref<const T> optional_value(variant_type<T> type) const TYPE_SAFE_LVALUE_REF
            noexcept
        {
            return has_value(type) ? type_safe::ref(storage_.get_union().value(type)) : nullptr;
        }

#if TYPE_SAFE_USE_REF_QUALIFIERS
        /// \group optional_value
        template <typename T>
            optional_xvalue_ref<T> optional_value(variant_type<T> type) && noexcept
        {
            return has_value(type) ? type_safe::xref(storage_.get_union().value(type)) : nullptr;
        }

        /// \group optional_value
        template <typename T>
        optional_xvalue_ref<const T> optional_value(variant_type<T> type) const && noexcept
        {
            return has_value(type) ? type_safe::xref(storage_.get_union().value(type)) : nullptr;
        }
#endif

        /// \returns If the variant currently stores an object of type `T`,
        /// returns a copy of that by copy (1)/move (2) constructing.
        /// Otherwise returns `other` converted to `T`.
        /// \throws Anything thrown by `T`s copy (1)/move (2) constructor or the converting constructor.
        /// \notes `T` must not necessarily be a type that can be stored in the variant./
        /// \notes This function does not participate in overload resolution,
        /// unless `T` is copy (1)/move (2) constructible and the fallback convertible to `T`.
        /// \group value_or
        /// \param 2
        /// \exclude
        template <typename T, typename U>
        T value_or(variant_type<T> type, U&& other,
                   typename std::enable_if<std::is_copy_constructible<T>::value
                                               && std::is_convertible<U&&, T>::value,
                                           int>::type = 0) const TYPE_SAFE_LVALUE_REF
        {
            return has_value(type) ? storage_.get_union().value(type) :
                                     static_cast<T>(std::forward<U>(other));
        }

#if TYPE_SAFE_USE_REF_QUALIFIERS
        /// \group value_or
        /// \param 2
        /// \exclude
        template <typename T, typename U>
        T value_or(variant_type<T> type, U&& other,
                   typename std::enable_if<std::is_move_constructible<T>::value
                                               && std::is_convertible<U&&, T>::value,
                                           int>::type = 0) &&
        {
            return has_value(type) ? std::move(storage_.get_union()).value(type) :
                                     static_cast<T>(std::forward<U>(other));
        }
#endif

        /// Maps a variant with a function.
        /// \returns A new variant of the same type.
        /// If the variant is empty, the result will be empty as well.
        /// Otherwise let the variant contain an object of type `T`.
        /// If the expression `std::forward<Functor>(value(variant_type<T>{}))` is well-formed,
        /// the result will contain the result returned by the function.
        /// If the type of the result cannot be stored in the variant,
        /// the program is ill-formed.
        /// If the expression is not well-formed,
        /// will contain a copy of the object.
        /// \throws Anything thrown by the function or copy/move constructor,
        /// in which case the variant will be left unchanged,
        /// unless the object was already moved into the function and modified there.
        /// \notes (1) will use the copy constructor, (2) will use the move constructor.
        /// The function does not participate in overload resolution,
        /// if copy (1)/move (2) constructors are not available for all types.
        /// \group map
        /// \param 1
        /// \exclude
        template <typename Functor,
                  typename = typename std::enable_if<traits::copy_constructible::value>::type>
        basic_variant map(Functor&& f) const TYPE_SAFE_LVALUE_REF
        {
            basic_variant result(force_empty{});
            if (!has_value())
                return result;
            detail::map_union<Functor&&, union_t>::map(result.storage_.get_union(),
                                                       storage_.get_union(),
                                                       std::forward<Functor>(f));
            DEBUG_ASSERT(result.has_value(), detail::assert_handler{});
            return result;
        }

#if TYPE_SAFE_USE_REF_QUALIFIERS
        /// \group map
        /// \param 1
        /// \exclude
        template <typename Functor,
                  typename = typename std::enable_if<traits::move_constructible::value>::type>
        basic_variant map(Functor&& f) &&
        {
            basic_variant result(force_empty{});
            if (!has_value())
                return result;
            detail::map_union<Functor&&, union_t>::map(result.storage_.get_union(),
                                                       std::move(storage_.get_union()),
                                                       std::forward<Functor>(f));
            DEBUG_ASSERT(result.has_value(), detail::assert_handler{});
            return result;
        }
#endif

    private:
        struct force_empty
        {
        };

        basic_variant(force_empty) noexcept
        {
        }

        detail::variant_storage<VariantPolicy, HeadT, TailT...> storage_;
    };

    /// \exclude
    template <class VariantPolicy, typename Head, typename... Types>
    constexpr typename basic_variant<VariantPolicy, Head, Types...>::type_id
        basic_variant<VariantPolicy, Head, Types...>::invalid_type;

//=== comparison ===//
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op, Expr, Expr2)                                                  \
    template <class VariantPolicy, typename... Types>                                              \
    bool operator Op(const basic_variant<VariantPolicy, Types...>& lhs, nullvar_t)                 \
    {                                                                                              \
        return (void)lhs, Expr;                                                                    \
    }                                                                                              \
    /** \group variant_comp_null */                                                                \
    template <class VariantPolicy, typename... Types>                                              \
    bool operator Op(nullvar_t, const basic_variant<VariantPolicy, Types...>& rhs)                 \
    {                                                                                              \
        return (void)rhs, Expr2;                                                                   \
    }

    /// Compares a [ts::basic_variant]() with [ts::nullvar]().
    ///
    /// A variant compares equal to `nullvar`, when it does not have a value.
    /// A variant compares never less to `nullvar`, `nullvar` compares less only if the variant has a value.
    /// The other comparisons behave accordingly.
    /// \group variant_comp_null
    /// \module variant
    TYPE_SAFE_DETAIL_MAKE_OP(==, !lhs.has_value(), !rhs.has_value())
    /// \group variant_comp_null
    TYPE_SAFE_DETAIL_MAKE_OP(!=, lhs.has_value(), rhs.has_value())
    /// \group variant_comp_null
    TYPE_SAFE_DETAIL_MAKE_OP(<, false, rhs.has_value())
    /// \group variant_comp_null
    TYPE_SAFE_DETAIL_MAKE_OP(<=, !lhs.has_value(), true)
    /// \group variant_comp_null
    TYPE_SAFE_DETAIL_MAKE_OP(>, lhs.has_value(), false)
    /// \group variant_comp_null
    TYPE_SAFE_DETAIL_MAKE_OP(>=, true, !rhs.has_value())

#undef TYPE_SAFE_DETAIL_MAKE_OP

    /// Compares a [ts::basic_variant]() with a value.
    ///
    /// A variant compares equal to a value, if it contains an object of the same type and the object compares equal.
    /// A variant compares less to a value,
    /// if - when it has a different type - the type id compares less than the type id of the value,
    /// or - when it has the same type - the object compares less to the value.
    /// The other comparisons behave accordingly.
    /// \notes The value must not necessarily have a type that can be stored in the variant.
    /// \group variant_comp_t
    /// \module variant
    template <class VariantPolicy, typename... Types, typename T>
    bool operator==(const basic_variant<VariantPolicy, Types...>& lhs, const T& rhs)
    {
        return lhs.value(variant_type<T>{}) == rhs;
    }
    /// \group variant_comp_t
    template <class VariantPolicy, typename... Types, typename T>
    bool operator==(const T& lhs, const basic_variant<VariantPolicy, Types...>& rhs)
    {
        return rhs == lhs;
    }

    /// \group variant_comp_t
    template <class VariantPolicy, typename... Types, typename T>
    bool operator!=(const basic_variant<VariantPolicy, Types...>& lhs, const T& rhs)
    {
        return !(lhs == rhs);
    }
    /// \group variant_comp_t
    template <class VariantPolicy, typename... Types, typename T>
    bool operator!=(const T& lhs, const basic_variant<VariantPolicy, Types...>& rhs)
    {
        return !(rhs == lhs);
    }

    /// \group variant_comp_t
    template <class VariantPolicy, typename... Types, typename T>
    bool operator<(const basic_variant<VariantPolicy, Types...>& lhs, const T& rhs)
    {
        constexpr auto id = basic_variant<VariantPolicy, Types...>::type_id(variant_type<T>{});
        if (lhs.type() != id)
            return lhs.type() < id;
        return lhs.value(variant_type<T>{}) < rhs;
    }
    /// \group variant_comp_t
    template <class VariantPolicy, typename... Types, typename T>
    bool operator<(const T& lhs, const basic_variant<VariantPolicy, Types...>& rhs)
    {
        constexpr auto id = basic_variant<VariantPolicy, Types...>::type_id(variant_type<T>{});
        if (id != rhs.type())
            return id < rhs.type();
        return lhs < rhs.value(variant_type<T>{});
    }

    /// \group variant_comp_t
    template <class VariantPolicy, typename... Types, typename T>
    bool operator<=(const basic_variant<VariantPolicy, Types...>& lhs, const T& rhs)
    {
        return !(rhs < lhs);
    }
    /// \group variant_comp_t
    template <class VariantPolicy, typename... Types, typename T>
    bool operator<=(const T& lhs, const basic_variant<VariantPolicy, Types...>& rhs)
    {
        return !(rhs < lhs);
    }

    /// \group variant_comp_t
    template <class VariantPolicy, typename... Types, typename T>
    bool operator>(const basic_variant<VariantPolicy, Types...>& lhs, const T& rhs)
    {
        return rhs < lhs;
    }
    /// \group variant_comp_t
    template <class VariantPolicy, typename... Types, typename T>
    bool operator>(const T& lhs, const basic_variant<VariantPolicy, Types...>& rhs)
    {
        return rhs < lhs;
    }

    /// \group variant_comp_t
    template <class VariantPolicy, typename... Types, typename T>
    bool operator>=(const basic_variant<VariantPolicy, Types...>& lhs, const T& rhs)
    {
        return !(lhs < rhs);
    }
    /// \group variant_comp_t
    template <class VariantPolicy, typename... Types, typename T>
    bool operator>=(const T& lhs, const basic_variant<VariantPolicy, Types...>& rhs)
    {
        return !(lhs < rhs);
    }

    /// \exclude
    namespace detail
    {
        template <class Variant>
        class compare_variant;

        template <class VariantPolicy, typename... Types>
        class compare_variant<basic_variant<VariantPolicy, Types...>>
        {
        public:
            static bool compare_equal(const basic_variant<VariantPolicy, Types...>& a,
                                      const basic_variant<VariantPolicy, Types...>& b)
            {
                if (!a.has_value())
                    // to be equal, b must not have value as well
                    return !b.has_value();
                auto idx = static_cast<std::size_t>(a.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), assert_handler{});
                return equal_callbacks[idx](a, b);
            }

            static bool compare_less(const basic_variant<VariantPolicy, Types...>& a,
                                     const basic_variant<VariantPolicy, Types...>& b)
            {
                if (!a.has_value())
                    // for a to be less than b,
                    // b must have a value
                    return b.has_value();
                auto idx = static_cast<std::size_t>(a.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), assert_handler{});
                return less_callbacks[idx](a, b);
            }

        private:
            template <typename T>
            static bool compare_equal_impl(const basic_variant<VariantPolicy, Types...>& a,
                                           const basic_variant<VariantPolicy, Types...>& b)
            {
                DEBUG_ASSERT(a.has_value(variant_type<T>{}), assert_handler{});
                return a.value(variant_type<T>{}) == b;
            }

            template <typename T>
            static bool compare_less_impl(const basic_variant<VariantPolicy, Types...>& a,
                                          const basic_variant<VariantPolicy, Types...>& b)
            {
                DEBUG_ASSERT(a.has_value(variant_type<T>{}), assert_handler{});
                return a.value(variant_type<T>{}) < b;
            }

            using callback_type = bool (*)(const basic_variant<VariantPolicy, Types...>&,
                                           const basic_variant<VariantPolicy, Types...>&);
            static constexpr callback_type equal_callbacks[] = {&compare_equal_impl<Types>...};
            static constexpr callback_type less_callbacks[]  = {&compare_less_impl<Types>...};
        };
        template <class VariantPolicy, typename... Types>
        constexpr typename compare_variant<basic_variant<VariantPolicy, Types...>>::callback_type
            compare_variant<basic_variant<VariantPolicy, Types...>>::equal_callbacks[];
        template <class VariantPolicy, typename... Types>
        constexpr typename compare_variant<basic_variant<VariantPolicy, Types...>>::callback_type
            compare_variant<basic_variant<VariantPolicy, Types...>>::less_callbacks[];
    } // namespace detail

    /// Compares two [ts::basic_variant]()s.
    ///
    /// They compare equal if both store the same type (or none) and the stored object compares equal.
    /// A variant is less than another if they store mismatched types and the type id of the first is less than the other,
    /// or if they store the same type and the stored object compares less.
    /// The other comparisons behave accordingly.
    /// \module variant
    /// \group variant_comp
    template <class VariantPolicy, typename... Types>
    bool operator==(const basic_variant<VariantPolicy, Types...>& lhs,
                    const basic_variant<VariantPolicy, Types...>& rhs)
    {
        return detail::compare_variant<basic_variant<VariantPolicy, Types...>>::compare_equal(lhs,
                                                                                              rhs);
    }

    /// \group variant_comp
    template <class VariantPolicy, typename... Types>
    bool operator!=(const basic_variant<VariantPolicy, Types...>& lhs,
                    const basic_variant<VariantPolicy, Types...>& rhs)
    {
        return !(lhs == rhs);
    }

    /// \group variant_comp
    template <class VariantPolicy, typename... Types>
    bool operator<(const basic_variant<VariantPolicy, Types...>& lhs,
                   const basic_variant<VariantPolicy, Types...>& rhs)
    {
        return detail::compare_variant<basic_variant<VariantPolicy, Types...>>::compare_less(lhs,
                                                                                             rhs);
    }

    /// \group variant_comp
    template <class VariantPolicy, typename... Types>
    bool operator<=(const basic_variant<VariantPolicy, Types...>& lhs,
                    const basic_variant<VariantPolicy, Types...>& rhs)
    {
        return !(rhs < lhs);
    }

    /// \group variant_comp
    template <class VariantPolicy, typename... Types>
    bool operator>(const basic_variant<VariantPolicy, Types...>& lhs,
                   const basic_variant<VariantPolicy, Types...>& rhs)
    {
        return rhs < lhs;
    }

    /// \group variant_comp
    template <class VariantPolicy, typename... Types>
    bool operator>=(const basic_variant<VariantPolicy, Types...>& lhs,
                    const basic_variant<VariantPolicy, Types...>& rhs)
    {
        return !(lhs < rhs);
    }

    /// \exclude
    namespace detail
    {
        template <bool AllowIncomplete, typename Visitor, class... Variants>
        class visit_variant_impl;

        template <bool AllowIncomplete, typename Visitor>
        class visit_variant_impl<AllowIncomplete, Visitor>
        {
        public:
            template <typename... Args>
            static auto call(Visitor&& visitor, Args&&... args)
                -> decltype(call_impl(0, std::forward<Visitor>(visitor),
                                      std::forward<Args>(args)...))
            {
                return call_impl(0, std::forward<Visitor>(visitor), std::forward<Args>(args)...);
            }

        private:
            template <typename... Args>
            static auto call_impl(int, Visitor&& visitor, Args&&... args)
                -> decltype(std::forward<Visitor>(visitor)(std::forward<Args>(args)...))
            {
                return std::forward<Visitor>(visitor)(std::forward<Args>(args)...);
            }

            template <typename... Args>
            static void call_impl(short, Visitor&& visitor, Args&&... args)
            {
                static_assert(AllowIncomplete, "visitor does not cover all possible combinations");
            }
        };

        template <bool AllowIncomplete, typename Visitor, class Variant, class... Rest>
        class visit_variant_impl<AllowIncomplete, Visitor, Variant, Rest...>
        {
        public:
            template <typename... Args>
            static auto call(Visitor&& visitor, Variant&& variant, Rest&&... rest, Args&&... args)
                -> decltype(call_type(std::forward<Visitor>(visitor), typename Variant::types{},
                                      std::forward<Variant>(variant), std::forward<Rest>(rest)...,
                                      std::forward<Args>(args)...))
            {
                return call_type(std::forward<Visitor>(visitor), typename Variant::types{},
                                 std::forward<Variant>(variant), std::forward<Rest>(rest)...,
                                 std::forward<Args>(args)...);
            }

        private:
            template <typename... Args>
            static auto call_type(Visitor&& visitor, variant_types<>, Variant&& variant,
                                  Rest&&... rest, Args&&... args)
                -> decltype(visit_variant_impl<AllowIncomplete, Visitor, Rest...>::call(
                    std::forward<Visitor>(visitor), std::forward<Rest>(rest)...,
                    std::forward<Args>(args)..., nullvar))
            {
                DEBUG_ASSERT(!variant.has_value(), assert_handler{});
                return visit_variant_impl<AllowIncomplete, Visitor,
                                          Rest...>::call(std::forward<Visitor>(visitor),
                                                         std::forward<Rest>(rest)...,
                                                         std::forward<Args>(args)..., nullvar);
            }

            template <typename T, typename... Args>
            static auto call_type_impl(Visitor&& visitor, Variant&& variant, Rest&&... rest,
                                       Args&&... args)
                -> decltype(visit_variant_impl<AllowIncomplete, Visitor, Rest...>::call(
                    std::forward<Visitor>(visitor), std::forward<Rest>(rest)...,
                    std::forward<Args>(args)...,
                    std::forward<Variant>(variant).value(variant_type<T>{})))
            {
                return visit_variant_impl<AllowIncomplete, Visitor,
                                          Rest...>::call(std::forward<Visitor>(visitor),
                                                         std::forward<Rest>(rest)...,
                                                         std::forward<Args>(args)...,
                                                         std::forward<Variant>(variant).value(
                                                             variant_type<T>{}));
            }

            template <typename Head, typename... Tail, typename... Args>
            static auto call_type(Visitor&& visitor, variant_types<Head, Tail...>,
                                  Variant&& variant, Rest&&... rest, Args&&... args)
                -> common_type_t<decltype(call_type_impl<Head>(
                                     std::forward<Visitor>(visitor), std::forward<Variant>(variant),
                                     std::forward<Rest>(rest)..., std::forward<Args>(args)...)),
                                 decltype(
                                     visit_variant_impl<AllowIncomplete, Visitor, Rest...>::call(
                                         std::forward<Visitor>(visitor),
                                         std::forward<Rest>(rest)..., std::forward<Args>(args)...,
                                         std::forward<Variant>(variant).value(
                                             variant_type<Head>{})))>
            {
                if (variant.has_value(variant_type<Head>{}))
                    return visit_variant_impl<AllowIncomplete, Visitor,
                                              Rest...>::call(std::forward<Visitor>(visitor),
                                                             std::forward<Rest>(rest)...,
                                                             std::forward<Args>(args)...,
                                                             std::forward<Variant>(variant).value(
                                                                 variant_type<Head>{}));
                else
                    return call_type(std::forward<Visitor>(visitor), variant_types<Tail...>{},
                                     std::forward<Variant>(variant), std::forward<Rest>(rest)...,
                                     std::forward<Args>(args)...);
            }
        };
    } // namespace detail
} // namespace type_safe

#endif // TYPE_SAFE_VARIANT_HPP_INCLUDED
