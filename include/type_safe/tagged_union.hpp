// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_TAGGED_UNION_HPP_INCLUDED
#define TYPE_SAFE_TAGGED_UNION_HPP_INCLUDED

#include <new>

#include <type_safe/detail/aligned_union.hpp>
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

        template <typename T, typename... Tail>
        struct get_type_index_impl<T, T, Tail...> : std::integral_constant<std::size_t, 0>
        {
        };

        template <typename T, typename Head, typename... Tail>
        struct get_type_index_impl<T, Head, Tail...>
            : std::integral_constant<std::size_t, 1 + get_type_index_impl<T, Tail...>::value>
        {
        };

        template <typename T, typename... Types>
        using get_type_index = get_type_index_impl<T, typename std::decay<Types>::type...>;
    } // namespace detail

    /// Tag type so no explicit template instantiation of function parameters is required.
    /// \module variant
    template <typename T>
    struct union_type
    {
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
            /// \effects Initializes it to an invalid value.
            constexpr type_id() noexcept : strong_typedef<type_id, std::size_t>(sizeof...(Types))
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

        /// \notes Does not destroy the currently xtored type.
        ~tagged_union() noexcept = default;

        tagged_union(const tagged_union&) = delete;
        tagged_union& operator=(const tagged_union&) = delete;

        //=== modifiers ===//
        /// \effects Creates a new object of given type by perfectly forwarding `args`.
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
            DEBUG_ASSERT(cur_type_ == type, detail::assert_handler{},
                         "different type stored in union");
        }

        using storage_t = detail::aligned_union_t<Types...>;
        storage_t storage_;
        type_id   cur_type_;
    };

    /// \exclude
    template <typename... Types>
    constexpr typename tagged_union<Types...>::type_id tagged_union<Types...>::invalid_type;
} // namespace type_safe

#endif // TYPE_SAFE_TAGGED_UNION_HPP_INCLUDED
