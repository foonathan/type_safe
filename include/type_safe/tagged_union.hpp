// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_TAGGED_UNION_HPP_INCLUDED
#define TYPE_SAFE_TAGGED_UNION_HPP_INCLUDED

#include <new>

#include <type_safe/detail/aligned_union.hpp>
#include <type_safe/detail/all_of.hpp>
#include <type_safe/config.hpp>
#include <type_safe/strong_typedef.hpp>

namespace type_safe
{
    /// \exclude
    namespace detail
    {
        struct union_type_id : strong_typedef<union_type_id, std::size_t>,
                               strong_typedef_op::equality_comparison<union_type_id, bool>,
                               strong_typedef_op::relational_comparison<union_type_id, bool>
        {
            using strong_typedef<union_type_id, std::size_t>::strong_typedef;
        };

        //=== get_type_index ==//
        template <typename T, typename... Ts>
        struct get_type_index_impl;

        // type not found at all
        template <typename T>
        struct get_type_index_impl<T> : std::integral_constant<std::size_t, 0>
        {
        };

        // found type at the beginning
        template <typename T, typename... Tail>
        struct get_type_index_impl<T, T, Tail...> : std::integral_constant<std::size_t, 1>
        {
        };

        // type not in the beginning
        template <typename T, typename Head, typename... Tail>
        struct get_type_index_impl<T, Head, Tail...>
            : std::integral_constant<std::size_t, get_type_index_impl<T, Tail...>::value == 0u ?
                                                      0u :
                                                      1 + get_type_index_impl<T, Tail...>::value>
        {
        };

        template <typename T, typename... Types>
        using get_type_index = get_type_index_impl<T, typename std::decay<Types>::type...>;

        template <class Union>
        class destroy_union;
        template <class Union>
        class copy_union;
        template <class Union>
        class move_union;
    } // namespace detail

    /// Tag type so no explicit template instantiation of function parameters is required.
    /// \module variant
    template <typename T>
    struct union_type
    {
        constexpr union_type()
        {
        }
    };

    /// Very basic typelist.
    /// \module variant
    template <typename... Ts>
    struct union_types
    {
    };

    /// A tagged union.
    ///
    /// It is much like a plain old C `union`,
    /// but remembers which type it currently stores.
    /// It can either store one of the given types or no type at all.
    /// \notes Like the C `union` it does not automatically destroy the currently stored type,
    /// and copy operations are deleted.
    /// \module variant
    template <typename... Types>
    class tagged_union
    {
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ < 5
        // does not have is_trivially_copyable
        using trivial = detail::all_of<std::is_trivial<Types>::value...>;
#else
        using trivial = detail::all_of<std::is_trivially_copyable<Types>::value...>;
#endif

        template <class Union>
        friend class detail::destroy_union;
        template <class Union>
        friend class detail::copy_union;
        template <class Union>
        friend class detail::move_union;

    public:
        using types = union_types<typename std::decay<Types>::type...>;

        /// The id of a type.
        ///
        /// It is a [ts::strong_typedef]() for `std::size_t`
        /// and provides equality and relational comparison.
        class type_id : public strong_typedef<type_id, std::size_t>,
                        public strong_typedef_op::equality_comparison<type_id, bool>,
                        public strong_typedef_op::relational_comparison<type_id, bool>
        {
        public:
            /// \returns `true` if `T` is a valid type, `false` otherwise.
            template <typename T>
            static constexpr bool is_valid()
            {
                return detail::get_type_index<T, Types...>::value != 0u;
            }

            /// \effects Initializes it to an invalid value.
            /// \notes The invalid value compares less than all valid values.
            constexpr type_id() noexcept : strong_typedef<type_id, std::size_t>(0u)
            {
            }

            /// \effects Initializes it to the value of the type `T`.
            /// If `T` is not one of the types of the union types,
            /// it will be the same as the default constructor.
            template <typename T>
            constexpr type_id(union_type<T>) noexcept
            : type_id(detail::get_type_index<T, Types...>::value)
            {
            }

            /// \returns `true` if the id is valid,
            /// `false` otherwise.
            explicit operator bool() const noexcept
            {
                return *this != type_id();
            }

        private:
            explicit constexpr type_id(std::size_t value)
            : strong_typedef<type_id, std::size_t>(value)
            {
            }
        };

        /// A global invalid type id object.
        static constexpr type_id invalid_type = type_id();

        //=== constructors/destructors/assignment ===//
        tagged_union() noexcept = default;

        /// \notes Does not destroy the currently stored type.
        ~tagged_union() noexcept = default;

        tagged_union(const tagged_union&) = delete;
        tagged_union& operator=(const tagged_union&) = delete;

        //=== modifiers ===//
        /// \effects Creates a new object of given type by perfectly forwarding `args`.
        /// \throws Anything thrown by `T`s constructor,
        /// in which case the union will stay empty.
        /// \requires The union must currently be empty.
        /// and `T` must be a valid type and constructible from the arguments.
        template <typename T, typename... Args>
        void emplace(union_type<T>, Args&&... args)
        {
            constexpr auto index = type_id(union_type<T>{});
            static_assert(index != invalid_type, "T must not be stored in variant");
            static_assert(std::is_constructible<T, Args&&...>::value,
                          "T not constructible from arguments");

            ::new (get_memory()) T(std::forward<Args>(args)...);
            cur_type_ = index;
        }

        /// \effects Destroys the currently stored type by calling its destructor,
        /// and setting the union to the empty state.
        /// \requires The union must currently store an object of the given type.
        template <typename T>
        void destroy(union_type<T> type) noexcept
        {
            check(type);
            value(type).~T();
            cur_type_ = invalid_type;
        }

        //=== accessors ===//
        /// \returns The [*type_id]() of the type currently stored,
        /// or [*invalid_type_id]() if there is none.
        const type_id& type() const noexcept
        {
            return cur_type_;
        }

        /// \returns `true` if there is a type stored,
        /// `false` otherwise.
        bool has_value() const noexcept
        {
            return type() != invalid_type;
        }

        /// \returns A (`const`) lvalue/rvalue reference to the currently stored type.
        /// \requires The union must currently store an object of the given type.
        /// \group value
        template <typename T>
        T& value(union_type<T> type) TYPE_SAFE_LVALUE_REF noexcept
        {
            check(type);
            return *static_cast<T*>(get_memory());
        }

        /// \group value
        template <typename T>
        const T& value(union_type<T> type) const TYPE_SAFE_LVALUE_REF noexcept
        {
            check(type);
            return *static_cast<const T*>(get_memory());
        }

#if TYPE_SAFE_USE_REF_QUALIFIERS
        /// \group value
        template <typename T>
            T&& value(union_type<T> type) && noexcept
        {
            check(type);
            return std::move(*static_cast<T*>(get_memory()));
        }

        /// \group value
        template <typename T>
        const T&& value(union_type<T> type) const && noexcept
        {
            check(type);
            return std::move(*static_cast<const T*>(get_memory()));
        }
#endif

    private:
        void* get_memory() noexcept
        {
            return static_cast<void*>(&storage_);
        }

        const void* get_memory() const noexcept
        {
            return static_cast<const void*>(&storage_);
        }

        template <typename T>
        void check(union_type<T> type) const noexcept
        {
            DEBUG_ASSERT(cur_type_ == type, detail::precondition_error_handler{},
                         "different type stored in union");
        }

        using storage_t = detail::aligned_union_t<Types...>;
        storage_t storage_;
        type_id   cur_type_;
    };

    /// \exclude
    template <typename... Types>
    constexpr typename tagged_union<Types...>::type_id tagged_union<Types...>::invalid_type;

    /// \exclude
    namespace detail
    {
        template <class Union>
        class destroy_union;

        template <typename... Types>
        class destroy_union<tagged_union<Types...>>
        {
        public:
            static void destroy(tagged_union<Types...>& u) noexcept
            {
                if (!tagged_union<Types...>::trivial::value)
                {
                    auto idx = static_cast<std::size_t>(u.type()) - 1u;
                    DEBUG_ASSERT(idx < sizeof...(Types), detail::assert_handler{});
                    callbacks[idx](u);
                }
            }

        private:
            template <typename T>
            static void destroy_impl(tagged_union<Types...>& u) noexcept
            {
                u.destroy(union_type<T>{});
            }

            using callback_type                        = void (*)(tagged_union<Types...>&);
            static constexpr callback_type callbacks[] = {&destroy_impl<Types>...};
        };
        template <typename... Types>
        constexpr typename destroy_union<tagged_union<Types...>>::callback_type
            destroy_union<tagged_union<Types...>>::callbacks[];

        template <class Union>
        class copy_union;

        template <typename... Types>
        class copy_union<tagged_union<Types...>>
        {
        public:
            static void copy(tagged_union<Types...>& dest, const tagged_union<Types...>& org)
            {
                DEBUG_ASSERT(!dest.has_value(), detail::assert_handler{});

                if (tagged_union<Types...>::trivial::value)
                    dest.storage_ = org.storage_;
                else
                {
                    auto idx = static_cast<std::size_t>(org.type()) - 1u;
                    DEBUG_ASSERT(idx < sizeof...(Types), detail::assert_handler{});
                    callbacks[idx](dest, org);
                }
            }

        private:
            template <typename T>
            static void copy_impl(tagged_union<Types...>& dest, const tagged_union<Types...>& org)
            {
                dest.emplace(union_type<T>{}, org.value(union_type<T>{}));
            }

            using callback_type = void (*)(tagged_union<Types...>&, const tagged_union<Types...>&);
            static constexpr callback_type callbacks[] = {&copy_impl<Types>...};
        };
        template <typename... Types>
        constexpr typename copy_union<tagged_union<Types...>>::callback_type
            copy_union<tagged_union<Types...>>::callbacks[];

        template <class Union>
        class move_union;

        template <typename... Types>
        class move_union<tagged_union<Types...>>
        {
        public:
            static void move(tagged_union<Types...>& dest, tagged_union<Types...>&& org)
            {
                DEBUG_ASSERT(!dest.has_value(), detail::assert_handler{});

                if (tagged_union<Types...>::trivial::value)
                    dest.storage_ = org.storage_;
                else
                {
                    auto idx = static_cast<std::size_t>(org.type()) - 1u;
                    DEBUG_ASSERT(idx < sizeof...(Types), detail::assert_handler{});
                    callbacks[idx](dest, std::move(org));
                }
            }

        private:
            template <typename T>
            static void move_impl(tagged_union<Types...>& dest, tagged_union<Types...>&& org)
            {
                dest.emplace(union_type<T>{}, std::move(org.value(union_type<T>{})));
            }

            using callback_type = void (*)(tagged_union<Types...>&, tagged_union<Types...>&&);
            static constexpr callback_type callbacks[] = {&move_impl<Types>...};
        };
        template <typename... Types>
        constexpr typename move_union<tagged_union<Types...>>::callback_type
            move_union<tagged_union<Types...>>::callbacks[];
    } // namespace detail

    /// \effects Destroys the type currently stored in the [ts::tagged_union](),
    /// by calling `u.destroy(union_type<T>{})`.
    /// \requires The union must currently store a type,
    /// i.e. `has_value()` must return `true`.
    /// \module variant
    template <typename... Types>
    void destroy(tagged_union<Types...>& u) noexcept
    {
        detail::destroy_union<tagged_union<Types...>>::destroy(u);
    }

    /// \effects Copies the type currently stored in one [ts::tagged_union]() to another.
    /// This is equivalent to calling `dest.emplace(union_type<T>{}, org.value(union_type<T>{}))` (1)/
    /// `dest.emplace(union_type<T>{}, std::move(org).value(union_type<T>{}))` (2),
    /// where `T` is the type currently stored in the union.
    /// \throws Anything by the copy/move constructor in which case nothing has changed.
    /// \requires `dest` must not store a type, but `org` must have one,
    /// and all types must be copyable/moveable.
    /// \group union_copy_move
    /// \module variant
    /// \unique_name copy_union
    template <typename... Types>
    void copy(tagged_union<Types...>& dest, const tagged_union<Types...>& org)
    {
        detail::copy_union<tagged_union<Types...>>::copy(dest, org);
    }

    /// \group union_copy_move
    /// \module variant
    /// \unique_name move_union
    template <typename... Types>
    void move(tagged_union<Types...>& dest, tagged_union<Types...>&& org)
    {
        detail::move_union<tagged_union<Types...>>::move(dest, std::move(org));
    }
} // namespace type_safe

#endif // TYPE_SAFE_TAGGED_UNION_HPP_INCLUDED
