// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_INTEGER_HPP_INCLUDED
#define TYPE_SAFE_INTEGER_HPP_INCLUDED

#include <limits>
#include <type_traits>

#include <type_safe/detail/force_inline.hpp>
#include <type_safe/assert.hpp>

namespace type_safe
{
    template <typename IntegerT>
    class integer;

    /// \exclude
    namespace detail
    {
        template <typename T>
        struct is_integer : std::integral_constant<bool, std::is_integral<T>::value
                                                             && !std::is_same<T, bool>::value
                                                             && !std::is_same<T, char>::value>
        {
        };

        template <typename From, typename To>
        struct is_safe_integer_conversion
            : std::integral_constant<bool, detail::is_integer<From>::value
                                               && detail::is_integer<To>::value
                                               && sizeof(From) <= sizeof(To)
                                               && std::is_signed<From>::value
                                                      == std::is_signed<To>::value>
        {
        };

        template <typename From, typename To>
        using enable_safe_integer_conversion =
            typename std::enable_if<is_safe_integer_conversion<From, To>::value>::type;

        template <typename From, typename To>
        using fallback_safe_integer_conversion =
            typename std::enable_if<!is_safe_integer_conversion<From, To>::value>::type;

        template <typename A, typename B>
        struct is_safe_integer_comparision
            : std::integral_constant<bool, is_safe_integer_conversion<A, B>::value
                                               || is_safe_integer_conversion<B, A>::value>
        {
        };

        template <typename A, typename B>
        using enable_safe_integer_comparision =
            typename std::enable_if<is_safe_integer_comparision<A, B>::value>::type;

        template <typename A, typename B>
        using fallback_safe_integer_comparision =
            typename std::enable_if<!is_safe_integer_comparision<A, B>::value>::type;

        template <typename A, typename B>
        struct is_safe_integer_operation
            : std::integral_constant<bool,
                                     detail::is_integer<A>::value && detail::is_integer<B>::value
                                         && std::is_signed<A>::value == std::is_signed<B>::value>
        {
        };

        template <typename A, typename B>
        struct integer_result_type
            : std::enable_if<is_safe_integer_operation<A, B>::value,
                             typename std::conditional<sizeof(A) < sizeof(B), B, A>::type>
        {
        };

        template <typename A, typename B>
        using integer_result_t = typename integer_result_type<A, B>::type;

        template <typename A, typename B>
        using fallback_integer_result =
            typename std::enable_if<!is_safe_integer_operation<A, B>::value>::type;
    } // namespace detail

    /// A type safe integer class.
    ///
    /// This is a tiny, no overhead wrapper over a standard integer type.
    /// It behaves exactly like the built-in types except that narrowing conversions are not allowed.
    /// It also checks against `unsigned` underflow in debug mode.
    ///
    /// A conversion is considered safe if both integer types have the same signedness
    /// and the size of the value being converted is less than or equal to the destination size.
    ///
    /// \requires `IntegerT` must be an integral type except `bool` and `char` (use `signed char`/`unsigned char`).
    /// \notes It intentionally does not provide the bitwise operations.
    template <typename IntegerT>
    class integer
    {
        static_assert(detail::is_integer<IntegerT>::value, "must be a real integer type");

    public:
        using integer_type = IntegerT;

        //=== constructors ===//
        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE constexpr integer(const T& val) noexcept : value_(val)
        {
        }

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE constexpr integer(const integer<T>& val) noexcept
            : value_(static_cast<T>(val))
        {
        }

        template <typename T, typename = detail::fallback_safe_integer_conversion<T, integer_type>>
        constexpr integer(T) = delete;

        //=== assignment ===//
        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator=(const T& val) noexcept
        {
            value_ = val;
            return *this;
        }

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator=(const integer<T>& val) noexcept
        {
            value_ = static_cast<T>(val);
            return *this;
        }

        template <typename T, typename = detail::fallback_safe_integer_conversion<T, integer_type>>
        integer& operator=(T) = delete;

        //=== conversion back ===//
        TYPE_SAFE_FORCE_INLINE explicit constexpr operator integer_type() const noexcept
        {
            return value_;
        }

        //=== unary operators ===//
        TYPE_SAFE_FORCE_INLINE constexpr integer operator+() const noexcept
        {
            return *this;
        }

        TYPE_SAFE_FORCE_INLINE constexpr integer operator-() const noexcept
        {
            static_assert(std::is_signed<integer_type>::value,
                          "cannot call unary minus on unsigned integer");
            return -value_;
        }

        TYPE_SAFE_FORCE_INLINE integer& operator++() noexcept
        {
            ++value_;
            return *this;
        }

        TYPE_SAFE_FORCE_INLINE integer operator++(int)noexcept
        {
            auto res = *this;
            ++value_;
            return res;
        }

        TYPE_SAFE_FORCE_INLINE integer& operator--() noexcept
        {
            DEBUG_ASSERT(std::is_signed<integer_type>::value || value_ > integer_type(0),
                         assert_handler{}, "underflow detected");
            --value_;
            return *this;
        }

        TYPE_SAFE_FORCE_INLINE integer operator--(int)noexcept
        {
            DEBUG_ASSERT(std::is_signed<integer_type>::value || value_ > integer_type(0),
                         assert_handler{}, "underflow detected");
            auto res = *this;
            --value_;
            return res;
        }

//=== compound assignment ====//
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>      \
    TYPE_SAFE_FORCE_INLINE integer& operator Op(const T& other) noexcept                           \
    {                                                                                              \
        return *this Op integer<T>(other);                                                         \
    }                                                                                              \
    template <typename T, typename = detail::fallback_safe_integer_conversion<T, integer_type>>    \
    integer& operator Op(integer<T>) = delete;                                                     \
    template <typename T, typename = detail::fallback_safe_integer_conversion<T, integer_type>>    \
    integer& operator Op(T) = delete;

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator+=(const integer<T>& other) noexcept
        {
            value_ += static_cast<T>(other);
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(+=)

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator-=(const integer<T>& other) noexcept
        {
            DEBUG_ASSERT(std::is_signed<T>::value || value_ > static_cast<T>(other),
                         assert_handler{}, "underflow detected");
            value_ -= static_cast<T>(other);
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(-=)

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator*=(const integer<T>& other) noexcept
        {
            value_ *= static_cast<T>(other);
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(*=)

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator/=(const integer<T>& other) noexcept
        {
            value_ /= static_cast<T>(other);
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(/=)

        template <typename T, typename = detail::enable_safe_integer_conversion<T, integer_type>>
        TYPE_SAFE_FORCE_INLINE integer& operator%=(const integer<T>& other) noexcept
        {
            value_ %= static_cast<T>(other);
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(%=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

    private:
        integer_type value_;
    };

    //=== operations ===//
    /// \exclude
    namespace detail
    {
        template <typename T>
        struct make_signed;

        template <typename T>
        struct make_signed<integer<T>>
        {
            using type = integer<typename std::make_signed<T>::type>;
        };

        template <typename T>
        struct make_unsigned;

        template <typename T>
        struct make_unsigned<integer<T>>
        {
            using type = integer<typename std::make_unsigned<T>::type>;
        };
    } // namespace detail

    /// [std::make_signed]() for [type_safe::integer]().
    template <class Integer>
    using make_signed_t = typename detail::make_signed<Integer>::type;

    /// \returns A new [type_safe::integer]() of the corresponding signed integer type.
    /// \requires The value of `i` must fit into signed type.
    template <typename Integer>
    TYPE_SAFE_FORCE_INLINE constexpr make_signed_t<integer<Integer>> make_signed(
        const integer<Integer>& i) noexcept
    {
        using result_type = typename std::make_signed<Integer>::type;
        return (constexpr_assert<result_type>(i <= static_cast<Integer>(
                                                       std::numeric_limits<result_type>::max()),
                                              DEBUG_ASSERT_CUR_SOURCE_LOCATION,
                                              "conversion would overflow"),
                integer<result_type>(static_cast<result_type>(static_cast<Integer>(i))));
    }

    /// [std::make_unsigned]() for [type_safe::integer]().
    template <class Integer>
    using make_unsigned_t = typename detail::make_unsigned<Integer>::type;

    /// \returns A new [type_safe::integer]() of the corresponding unsigned integer type.
    /// \requires The value of `i` must not be negative.
    template <typename Integer>
    TYPE_SAFE_FORCE_INLINE constexpr make_unsigned_t<integer<Integer>> make_unsigned(
        const integer<Integer>& i) noexcept
    {
        using result_type = typename std::make_unsigned<Integer>::type;
        return (constexpr_assert<result_type>(i >= 1, DEBUG_ASSERT_CUR_SOURCE_LOCATION,
                                              "conversion would underflow"),
                integer<result_type>(static_cast<result_type>(static_cast<Integer>(i))));
    }

//=== comparision ===//
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <typename A, typename B, typename = detail::enable_safe_integer_conversion<A, B>>     \
    TYPE_SAFE_FORCE_INLINE constexpr bool operator Op(const A& a, const integer<B>& b)             \
    {                                                                                              \
        return integer<A>(a) Op b;                                                                 \
    }                                                                                              \
    template <typename A, typename B, typename = detail::enable_safe_integer_conversion<A, B>>     \
    TYPE_SAFE_FORCE_INLINE constexpr bool operator Op(const integer<A>& a, const B& b)             \
    {                                                                                              \
        return a Op integer<B>(b);                                                                 \
    }                                                                                              \
    template <typename A, typename B, typename = detail::fallback_safe_integer_comparision<A, B>>  \
    constexpr bool operator Op(integer<A>, integer<B>) = delete;                                   \
    template <typename A, typename B, typename = detail::fallback_safe_integer_comparision<A, B>>  \
    constexpr bool operator Op(A, integer<B>) = delete;                                            \
    template <typename A, typename B, typename = detail::fallback_safe_integer_comparision<A, B>>  \
    constexpr bool operator Op(integer<A>, B) = delete;

    template <typename A, typename B, typename = detail::enable_safe_integer_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator==(const integer<A>& a,
                                                     const integer<B>& b) noexcept
    {
        return static_cast<A>(a) == static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(==)

    template <typename A, typename B, typename = detail::enable_safe_integer_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator!=(const integer<A>& a,
                                                     const integer<B>& b) noexcept
    {
        return static_cast<A>(a) != static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(!=)

    template <typename A, typename B, typename = detail::enable_safe_integer_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator<(const integer<A>& a,
                                                    const integer<B>& b) noexcept
    {
        return static_cast<A>(a) < static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(<)

    template <typename A, typename B, typename = detail::enable_safe_integer_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator<=(const integer<A>& a,
                                                     const integer<B>& b) noexcept
    {
        return static_cast<A>(a) <= static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(<=)

    template <typename A, typename B, typename = detail::enable_safe_integer_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator>(const integer<A>& a,
                                                    const integer<B>& b) noexcept
    {
        return static_cast<A>(a) > static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(>)

    template <typename A, typename B, typename = detail::enable_safe_integer_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator>=(const integer<A>& a,
                                                     const integer<B>& b) noexcept
    {
        return static_cast<A>(a) >= static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(>=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

//=== binary operations ===//
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <typename A, typename B>                                                              \
    TYPE_SAFE_FORCE_INLINE constexpr auto operator Op(const A& a, const integer<B>& b) noexcept    \
        ->integer<detail::integer_result_t<A, B>>                                                  \
    {                                                                                              \
        return integer<A>(a) Op b;                                                                 \
    }                                                                                              \
    template <typename A, typename B>                                                              \
    TYPE_SAFE_FORCE_INLINE constexpr auto operator Op(const integer<A>& a, const B& b) noexcept    \
        ->integer<detail::integer_result_t<A, B>>                                                  \
    {                                                                                              \
        return a Op integer<B>(b);                                                                 \
    }                                                                                              \
    template <typename A, typename B, typename = detail::fallback_integer_result<A, B>>            \
    constexpr int operator Op(integer<A>, integer<B>) noexcept = delete;                           \
    template <typename A, typename B, typename = detail::fallback_integer_result<A, B>>            \
    constexpr int operator Op(A, integer<B>) noexcept = delete;                                    \
    template <typename A, typename B, typename = detail::fallback_integer_result<A, B>>            \
    constexpr int operator Op(integer<A>, B) noexcept = delete;

    template <typename A, typename B>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator+(const integer<A>& a,
                                                    const integer<B>& b) noexcept
        -> integer<detail::integer_result_t<A, B>>
    {
        return static_cast<A>(a) + static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(+)

    template <typename A, typename B>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator-(const integer<A>& a,
                                                    const integer<B>& b) noexcept
        -> integer<detail::integer_result_t<A, B>>
    {
        return (constexpr_assert<A>(std::is_signed<A>::value || a >= b,
                                    DEBUG_ASSERT_CUR_SOURCE_LOCATION, "underflow detected"),
                static_cast<A>(a) - static_cast<B>(b));
    }
    TYPE_SAFE_DETAIL_MAKE_OP(-)

    template <typename A, typename B>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator*(const integer<A>& a,
                                                    const integer<B>& b) noexcept
        -> integer<detail::integer_result_t<A, B>>
    {
        return static_cast<A>(a) * static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(*)

    template <typename A, typename B>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator/(const integer<A>& a,
                                                    const integer<B>& b) noexcept
        -> integer<detail::integer_result_t<A, B>>
    {
        return static_cast<A>(a) / static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(/)

    template <typename A, typename B>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator%(const integer<A>& a,
                                                    const integer<B>& b) noexcept
        -> integer<detail::integer_result_t<A, B>>
    {
        return static_cast<A>(a) % static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(%)

#undef TYPE_SAFE_DETAIL_MAKE_OP
} // namespace type_safe

#endif // TYPE_SAFE_INTEGER_HPP_INCLUDED
