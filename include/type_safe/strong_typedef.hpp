// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_STRONG_TYPEDEF_HPP_INCLUDED
#define TYPE_SAFE_STRONG_TYPEDEF_HPP_INCLUDED

#include <iosfwd>
#include <iterator>
#include <type_traits>
#include <utility>

#include <type_safe/types.hpp>

namespace type_safe
{
    /// A strong typedef emulation.
    ///
    /// Unlike regular typedefs, this does create a new type and only allows explicit conversion from the underlying one.
    /// The `Tag` is used to differentiate between different strong typedefs to the same type.
    /// It is designed to be used as a base class and does not provide any operations by itself.
    /// Use the types in the `strong_typedef_op` namespace to generate operations
    /// and/or your own member functions.
    ///
    /// Example:
    /// ```cpp
    /// struct my_handle
    /// : strong_typedef<my_handle, void*>,
    ///   strong_typedef_op::equality_comparision<my_handle>
    /// {
    ///     using strong_typedef::strong_typedef;
    /// };
    ///
    /// struct my_int
    /// : strong_typedef<my_int, int>,
    ///   strong_typedef_op::integer_arithmetic<my_int>,
    ///   strong_typedef_op::equality_comparision<my_int>,
    ///   strong_typedef_op::relational_comparision<my_int>
    /// {
    ///     using strong_typedef::strong_typedef;
    /// };
    /// ```
    template <class Tag, typename T>
    class strong_typedef
    {
    public:
        /// \effects Value initializes the underlying value.
        constexpr strong_typedef() : value_()
        {
        }

        /// \effects Copy (1)/moves (2) the underlying value.
        /// \group value_ctor
        explicit constexpr strong_typedef(const T& value) : value_(value)
        {
        }

        /// \group value_ctor
        explicit constexpr strong_typedef(T&& value) noexcept(
            std::is_nothrow_move_constructible<T>::value)
        : value_(static_cast<T&&>(value)) // std::move() might not be constexpr
        {
        }

        /// \returns A reference to the stored underlying value.
        /// \group value_conv
        explicit operator T&() TYPE_SAFE_LVALUE_REF noexcept
        {
            return value_;
        }

        /// \group value_conv
        explicit constexpr operator const T&() const TYPE_SAFE_LVALUE_REF noexcept
        {
            return value_;
        }

#if TYPE_SAFE_USE_REF_QUALIFIERS
        /// \group value_conv
        explicit operator T &&() && noexcept
        {
            return std::move(value_);
        }

        /// \group value_conv
        explicit constexpr operator const T &&() const && noexcept
        {
            return std::move(value_);
        }
#endif

        friend void swap(strong_typedef& a, strong_typedef& b) noexcept
        {
            using std::swap;
            swap(static_cast<T&>(a), static_cast<T&>(b));
        }

    private:
        T value_;
    };

    /// \exclude
    namespace detail
    {
        template <class Tag, typename T>
        T underlying_type(strong_typedef<Tag, T>);
    } // namespace detail

    /// The underlying type of the [ts::strong_typedef]().
    /// \exclude target
    template <class StrongTypedef>
    using underlying_type = decltype(detail::underlying_type(std::declval<StrongTypedef>()));

    /// Accesses the underlying value.
    /// \returns A reference to the underlying value.
    /// \group strong_typedef_get
    template <class Tag, typename T>
    constexpr T& get(strong_typedef<Tag, T>& type) noexcept
    {
        return static_cast<T&>(type);
    }

    /// \group strong_typedef_get
    template <class Tag, typename T>
    constexpr const T& get(const strong_typedef<Tag, T>& type) noexcept
    {
        return static_cast<const T&>(type);
    }

    /// \group strong_typedef_get
    template <class Tag, typename T>
    constexpr T&& get(strong_typedef<Tag, T>&& type) noexcept
    {
        return static_cast<T&&>(static_cast<T&>(type));
    }

    /// \group strong_typedef_get
    template <class Tag, typename T>
    constexpr const T&& get(const strong_typedef<Tag, T>&& type) noexcept
    {
        return static_cast<const T&&>(static_cast<const T&>(type));
    }

    /// Some operations for [ts::strong_typedef]().
    ///
    /// They all generate operators forwarding to the underlying type,
    /// inherit from then in the typedef definition.
    namespace strong_typedef_op
    {
        template <class StrongTypedef, typename Result = bool_t>
        struct equality_comparison
        {
            friend constexpr Result operator==(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(lhs) == static_cast<const type&>(rhs);
            }

            friend constexpr Result operator!=(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                return !(lhs == rhs);
            }
        };

        template <class StrongTypedef, typename Other, typename Result = bool_t>
        struct mixed_equality_comparison
        {
            /// \group equal
            friend constexpr Result operator==(const StrongTypedef& lhs, const Other& rhs)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(lhs) == static_cast<const type&>(rhs);
            }

            /// \group equal
            friend constexpr Result operator==(const Other& lhs, const StrongTypedef& rhs)
            {
                return rhs == lhs;
            }

            /// \group not_equal
            friend constexpr Result operator!=(const StrongTypedef& lhs, const Other& rhs)
            {
                return !(lhs == rhs);
            }

            /// \group not_equal
            friend constexpr Result operator!=(const Other& lhs, const StrongTypedef& rhs)
            {
                return !(rhs == lhs);
            }
        };

        template <class StrongTypedef, typename Result = bool_t>
        struct relational_comparison
        {
            friend constexpr Result operator<(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(lhs) < static_cast<const type&>(rhs);
            }

            friend constexpr Result operator>(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                return rhs < lhs;
            }

            friend constexpr Result operator<=(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                return !(rhs < lhs);
            }

            friend constexpr Result operator>=(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                return !(lhs < rhs);
            }
        };

        template <class StrongTypedef, typename Other, typename Result = bool_t>
        struct mixed_relational_comparison
        {
            /// \group less
            friend constexpr Result operator<(const StrongTypedef& lhs, const Other& rhs)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(lhs) < static_cast<const type&>(rhs);
            }

            /// \group less
            friend constexpr Result operator<(const Other& lhs, const StrongTypedef& rhs)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(lhs) < static_cast<const type&>(rhs);
            }

            /// \group greater
            friend constexpr Result operator>(const StrongTypedef& lhs, const Other& rhs)
            {
                return rhs < lhs;
            }

            /// \group greater
            friend constexpr Result operator>(const Other& lhs, const StrongTypedef& rhs)
            {
                return rhs < lhs;
            }

            /// \group less_eq
            friend constexpr Result operator<=(const StrongTypedef& lhs, const Other& rhs)
            {
                return !(rhs < lhs);
            }

            /// \group less_eq
            friend constexpr Result operator<=(const Other& lhs, const StrongTypedef& rhs)
            {
                return !(rhs < lhs);
            }

            /// \group greater_equal
            friend constexpr Result operator>=(const StrongTypedef& lhs, const Other& rhs)
            {
                return !(lhs < rhs);
            }

            /// \group greater_equal
            friend constexpr Result operator>=(const Other& lhs, const StrongTypedef& rhs)
            {
                return !(lhs < rhs);
            }
        };

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Name, Op)                                                         \
    template <class StrongTypedef>                                                                 \
    struct Name                                                                                    \
    {                                                                                              \
        friend StrongTypedef& operator Op##=(StrongTypedef& lhs, const StrongTypedef& rhs)         \
        {                                                                                          \
            get(lhs) Op## = get(rhs);                                                              \
            return lhs;                                                                            \
        }                                                                                          \
                                                                                                   \
        friend StrongTypedef& operator Op##=(StrongTypedef& lhs, StrongTypedef&& rhs)              \
        {                                                                                          \
            get(lhs) Op## = get(std::move(rhs));                                                   \
            return lhs;                                                                            \
        }                                                                                          \
                                                                                                   \
        friend StrongTypedef&& operator Op##=(StrongTypedef&& lhs, const StrongTypedef& rhs)       \
        {                                                                                          \
            get(lhs) Op## = get(rhs);                                                              \
            return std::move(lhs);                                                                 \
        }                                                                                          \
                                                                                                   \
        friend StrongTypedef&& operator Op##=(StrongTypedef&& lhs, StrongTypedef&& rhs)            \
        {                                                                                          \
            get(lhs) Op## = get(std::move(rhs));                                                   \
            return std::move(lhs);                                                                 \
        }                                                                                          \
                                                                                                   \
        friend constexpr StrongTypedef operator Op(const StrongTypedef& lhs,                       \
                                                   const StrongTypedef& rhs)                       \
        {                                                                                          \
            return StrongTypedef(get(lhs) Op get(rhs));                                            \
        }                                                                                          \
                                                                                                   \
        friend constexpr StrongTypedef operator Op(StrongTypedef&& lhs, const StrongTypedef& rhs)  \
        {                                                                                          \
            return StrongTypedef(get(std::move(lhs)) Op get(rhs));                                 \
        }                                                                                          \
                                                                                                   \
        friend constexpr StrongTypedef operator Op(const StrongTypedef& lhs, StrongTypedef&& rhs)  \
        {                                                                                          \
            return StrongTypedef(get(lhs) Op get(std::move(rhs)));                                 \
        }                                                                                          \
                                                                                                   \
        friend constexpr StrongTypedef operator Op(StrongTypedef&& lhs, StrongTypedef&& rhs)       \
        {                                                                                          \
            return StrongTypedef(get(std::move(lhs)) Op get(std::move(rhs)));                      \
        }                                                                                          \
    };                                                                                             \
                                                                                                   \
    template <class StrongTypedef, typename Other>                                                 \
    struct mixed_##Name                                                                            \
    {                                                                                              \
        friend StrongTypedef& operator Op##=(StrongTypedef& lhs, const Other& other)               \
        {                                                                                          \
            using type    = underlying_type<StrongTypedef>;                                        \
            get(lhs) Op## = static_cast<const type&>(other);                                       \
            return lhs;                                                                            \
        }                                                                                          \
                                                                                                   \
        friend StrongTypedef&& operator Op##=(StrongTypedef&& lhs, const Other& other)             \
        {                                                                                          \
            using type    = underlying_type<StrongTypedef>;                                        \
            get(lhs) Op## = static_cast<const type&>(other);                                       \
            return std::move(lhs);                                                                 \
        }                                                                                          \
                                                                                                   \
        friend constexpr StrongTypedef operator Op(const StrongTypedef& lhs, const Other& rhs)     \
        {                                                                                          \
            using type = underlying_type<StrongTypedef>;                                           \
            return StrongTypedef(get(lhs) Op static_cast<const type&>(rhs));                       \
        }                                                                                          \
                                                                                                   \
        friend constexpr StrongTypedef operator Op(StrongTypedef&& lhs, const Other& rhs)          \
        {                                                                                          \
            using type = underlying_type<StrongTypedef>;                                           \
            return StrongTypedef(get(std::move(lhs)) Op static_cast<const type&>(rhs));            \
        }                                                                                          \
                                                                                                   \
        friend constexpr StrongTypedef operator Op(const Other& lhs, const StrongTypedef& rhs)     \
        {                                                                                          \
            using type = underlying_type<StrongTypedef>;                                           \
            return StrongTypedef(static_cast<const type&>(lhs) Op get(rhs));                       \
        }                                                                                          \
                                                                                                   \
        friend constexpr StrongTypedef operator Op(const Other& lhs, StrongTypedef&& rhs)          \
        {                                                                                          \
            using type = underlying_type<StrongTypedef>;                                           \
            return StrongTypedef(static_cast<const type&>(lhs) Op get(std::move(rhs)));            \
        }                                                                                          \
    };

        TYPE_SAFE_DETAIL_MAKE_OP(addition, +)
        TYPE_SAFE_DETAIL_MAKE_OP(subtraction, -)
        TYPE_SAFE_DETAIL_MAKE_OP(multiplication, *)
        TYPE_SAFE_DETAIL_MAKE_OP(division, /)
        TYPE_SAFE_DETAIL_MAKE_OP(modulo, %)

#undef TYPE_SAFE_DETAIL_MAKE_OP

        template <class StrongTypedef>
        struct increment
        {
            StrongTypedef& operator++()
            {
                using type = underlying_type<StrongTypedef>;
                ++static_cast<type&>(static_cast<StrongTypedef&>(*this));
                return static_cast<StrongTypedef&>(*this);
            }

            StrongTypedef operator++(int)
            {
                auto result = static_cast<StrongTypedef&>(*this);
                ++*this;
                return result;
            }
        };

        template <class StrongTypedef>
        struct decrement
        {
            StrongTypedef& operator--()
            {
                using type = underlying_type<StrongTypedef>;
                --static_cast<type&>(static_cast<StrongTypedef&>(*this));
                return static_cast<StrongTypedef&>(*this);
            }

            StrongTypedef operator--(int)
            {
                auto result = static_cast<StrongTypedef&>(*this);
                --*this;
                return result;
            }
        };

        template <class StrongTypedef>
        struct unary_plus
        {
            constexpr StrongTypedef operator+() const
            {
                using type = underlying_type<StrongTypedef>;
                return StrongTypedef(
                    +static_cast<const type&>(static_cast<const StrongTypedef&>(*this)));
            }
        };

        template <class StrongTypedef>
        struct unary_minus
        {
            constexpr StrongTypedef operator-() const
            {
                using type = underlying_type<StrongTypedef>;
                return StrongTypedef(
                    -static_cast<const type&>(static_cast<const StrongTypedef&>(*this)));
            }
        };

        template <class StrongTypedef>
        struct integer_arithmetic : unary_plus<StrongTypedef>,
                                    unary_minus<StrongTypedef>,
                                    addition<StrongTypedef>,
                                    subtraction<StrongTypedef>,
                                    multiplication<StrongTypedef>,
                                    division<StrongTypedef>,
                                    modulo<StrongTypedef>,
                                    increment<StrongTypedef>,
                                    decrement<StrongTypedef>
        {
        };

        template <class StrongTypedef>
        struct floating_point_arithmetic : unary_plus<StrongTypedef>,
                                           unary_minus<StrongTypedef>,
                                           addition<StrongTypedef>,
                                           subtraction<StrongTypedef>,
                                           multiplication<StrongTypedef>,
                                           division<StrongTypedef>
        {
        };

        template <class StrongTypedef, typename Result, typename ResultPtr = Result*,
                  typename ResultConstPtr = const Result*>
        struct dereference
        {
            /// \group deref
            Result& operator*()
            {
                using type = underlying_type<StrongTypedef>;
                return *static_cast<type&>(static_cast<StrongTypedef&>(*this));
            }

            /// \group deref
            const Result& operator*() const
            {
                using type = underlying_type<StrongTypedef>;
                return *static_cast<const type&>(static_cast<const StrongTypedef&>(*this));
            }

            /// \group pointer
            ResultPtr operator->()
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<type&>(static_cast<StrongTypedef&>(*this));
            }

            /// \group pointer
            ResultConstPtr operator->() const
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(static_cast<const StrongTypedef&>(*this));
            }
        };

        template <class StrongTypedef, typename Result, typename Index = std::size_t>
        struct array_subscript
        {
            /// \group access
            Result& operator[](const Index& i)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<type&>(static_cast<StrongTypedef&>(*this))[i];
            }

            /// \group access
            const Result& operator[](const Index& i) const
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(static_cast<const StrongTypedef&>(*this))[i];
            }
        };

        template <class StrongTypedef, class Category, typename T,
                  typename Distance = std::ptrdiff_t>
        struct iterator : dereference<StrongTypedef, T, T*, const T*>, increment<StrongTypedef>
        {
            using iterator_category = Category;
            using value_type        = T;
            using distance_type     = Distance;
            using pointer           = T*;
            using reference         = T&;
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct input_iterator : iterator<StrongTypedef, std::input_iterator_tag, T, Distance>,
                                equality_comparison<StrongTypedef, bool>
        {
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct output_iterator : iterator<StrongTypedef, std::output_iterator_tag, T, Distance>
        {
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct forward_iterator : input_iterator<StrongTypedef, T, Distance>
        {
            using iterator_category = std::forward_iterator_tag;
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct bidirectional_iterator : forward_iterator<StrongTypedef, T, Distance>,
                                        decrement<StrongTypedef>
        {
            using iterator_category = std::bidirectional_iterator_tag;
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct random_access_iterator : bidirectional_iterator<StrongTypedef, T, Distance>,
                                        array_subscript<StrongTypedef, T, Distance>,
                                        relational_comparison<StrongTypedef, bool>
        {
            using iterator_category = std::random_access_iterator_tag;

            StrongTypedef& operator+=(const Distance& d)
            {
                using type = underlying_type<StrongTypedef>;
                static_cast<type&>(static_cast<StrongTypedef&>(*this)) += d;
                return static_cast<StrongTypedef&>(*this);
            }

            StrongTypedef& operator-=(const Distance& d)
            {
                using type = underlying_type<StrongTypedef>;
                static_cast<type&>(static_cast<StrongTypedef&>(*this)) -= d;
                return static_cast<StrongTypedef&>(*this);
            }

            /// \group plus_dist
            friend StrongTypedef operator+(const StrongTypedef& iter, const Distance& n)
            {
                using type = underlying_type<StrongTypedef>;
                return StrongTypedef(static_cast<const type&>(iter) + n);
            }

            /// \group plus_dist
            friend StrongTypedef operator+(const Distance& n, const StrongTypedef& iter)
            {
                return iter + n;
            }

            friend StrongTypedef operator-(const StrongTypedef& iter, const Distance& n)
            {
                using type = underlying_type<StrongTypedef>;
                return StrongTypedef(static_cast<const type&>(iter) - n);
            }

            friend Distance operator-(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(lhs) - static_cast<const type&>(rhs);
            }
        };

        template <class StrongTypedef>
        struct input_operator
        {
            template <typename Char, class CharTraits>
            friend std::basic_istream<Char, CharTraits>& operator>>(
                std::basic_istream<Char, CharTraits>& in, StrongTypedef& val)
            {
                using type = underlying_type<StrongTypedef>;
                return in >> static_cast<type&>(val);
            }
        };

        template <class StrongTypedef>
        struct output_operator
        {
            template <typename Char, class CharTraits>
            friend std::basic_ostream<Char, CharTraits>& operator<<(
                std::basic_ostream<Char, CharTraits>& out, const StrongTypedef& val)
            {
                using type = underlying_type<StrongTypedef>;
                return out << static_cast<const type&>(val);
            }
        };
    } // namespace strong_typedef_op
} // namespace type_safe

#endif // TYPE_SAFE_STRONG_TYPEDEF_HPP_INCLUDED
