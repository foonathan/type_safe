// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
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
        strong_typedef() : value_()
        {
        }

        explicit strong_typedef(const T& value) : value_(value)
        {
        }

        explicit strong_typedef(T&& value) noexcept(std::is_nothrow_move_constructible<T>::value)
        : value_(std::move(value))
        {
        }

        explicit operator T&() noexcept
        {
            return value_;
        }

        explicit operator const T&() const noexcept
        {
            return value_;
        }

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

    template <class StrongTypedef>
    using underlying_type = decltype(detail::underlying_type(std::declval<StrongTypedef>()));

    namespace strong_typedef_op
    {
        template <class StrongTypedef, typename Result = bool_t>
        struct equality_comparision
        {
            friend Result operator==(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(lhs) == static_cast<const type&>(rhs);
            }

            friend Result operator!=(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                return !(lhs == rhs);
            }
        };

        template <class StrongTypedef, typename Result = bool_t>
        struct relational_comparision
        {
            friend Result operator<(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(lhs) < static_cast<const type&>(rhs);
            }

            friend Result operator>(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                return rhs < lhs;
            }

            friend Result operator<=(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                return !(rhs < lhs);
            }

            friend Result operator>=(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                return !(lhs < rhs);
            }
        };

#define TYPE_SAFE_DETAIL_MAKE_OP(Name, Op)                                                         \
    template <class StrongTypedef>                                                                 \
    struct Name                                                                                    \
    {                                                                                              \
        friend StrongTypedef& operator Op##=(StrongTypedef& lhs, const StrongTypedef& rhs)         \
        {                                                                                          \
            using type                   = underlying_type<StrongTypedef>;                         \
            static_cast<type&>(lhs) Op## = static_cast<const type&>(rhs);                          \
            return lhs;                                                                            \
        }                                                                                          \
                                                                                                   \
        friend StrongTypedef operator Op(const StrongTypedef& lhs, const StrongTypedef& rhs)       \
        {                                                                                          \
            using type = underlying_type<StrongTypedef>;                                           \
            return StrongTypedef(static_cast<const type&>(lhs) Op static_cast<const type&>(rhs));  \
        }                                                                                          \
    };                                                                                             \
                                                                                                   \
    template <class StrongTypedef, typename Other>                                                 \
    struct mixed_##Name                                                                            \
    {                                                                                              \
        friend StrongTypedef& operator Op##=(StrongTypedef& lhs, const Other& other)               \
        {                                                                                          \
            using type                   = underlying_type<StrongTypedef>;                         \
            static_cast<type&>(lhs) Op## = static_cast<const type&>(other);                        \
            return lhs;                                                                            \
        }                                                                                          \
                                                                                                   \
        friend StrongTypedef operator Op(const StrongTypedef& lhs, const Other& rhs)               \
        {                                                                                          \
            using type = underlying_type<StrongTypedef>;                                           \
            return StrongTypedef(static_cast<const type&>(lhs) Op rhs);                            \
        }                                                                                          \
                                                                                                   \
        friend StrongTypedef operator Op(const Other& lhs, const StrongTypedef& rhs)               \
        {                                                                                          \
            using type = underlying_type<StrongTypedef>;                                           \
            return StrongTypedef(lhs Op static_cast<const type&>(rhs));                            \
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
            StrongTypedef operator+() const
            {
                using type = underlying_type<StrongTypedef>;
                return StrongTypedef(
                    +static_cast<const type&>(static_cast<const StrongTypedef&>(*this)));
            }
        };

        template <class StrongTypedef>
        struct unary_minus
        {
            StrongTypedef operator-() const
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
            Result& operator*()
            {
                using type = underlying_type<StrongTypedef>;
                return *static_cast<type&>(static_cast<StrongTypedef&>(*this));
            }

            const Result& operator*() const
            {
                using type = underlying_type<StrongTypedef>;
                return *static_cast<const type&>(static_cast<const StrongTypedef&>(*this));
            }

            ResultPtr operator->()
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<type&>(static_cast<StrongTypedef&>(*this));
            }

            ResultConstPtr operator->() const
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(static_cast<const StrongTypedef&>(*this));
            }
        };

        template <class StrongTypedef, typename Result, typename Index = std::size_t>
        struct array_subscript
        {
            Result& operator[](const Index& i)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<type&>(static_cast<StrongTypedef&>(*this))[i];
            }

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
                                equality_comparision<StrongTypedef, bool>
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
                                        relational_comparision<StrongTypedef, bool>
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

            friend StrongTypedef operator+(const StrongTypedef& iter, const Distance& n)
            {
                using type = underlying_type<StrongTypedef>;
                return StrongTypedef(static_cast<const type&>(iter) + n);
            }

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
