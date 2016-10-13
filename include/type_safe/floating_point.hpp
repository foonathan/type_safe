// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_FLOATING_POINT_HPP_INCLUDED
#define TYPE_SAFE_FLOATING_POINT_HPP_INCLUDED

#include <iosfwd>
#include <type_traits>

#include <type_safe/detail/force_inline.hpp>

namespace type_safe
{
    template <typename FloatT>
    class floating_point;

    /// \exclude
    namespace detail
    {
        template <typename From, typename To>
        struct is_safe_floating_point_conversion
            : std::integral_constant<bool, std::is_floating_point<From>::value
                                               && std::is_floating_point<To>::value
                                               && sizeof(From) <= sizeof(To)>
        {
        };

        template <typename From, typename To>
        using enable_safe_floating_point_conversion =
            typename std::enable_if<is_safe_floating_point_conversion<From, To>::value>::type;

        template <typename From, typename To>
        using fallback_safe_floating_point_conversion =
            typename std::enable_if<!is_safe_floating_point_conversion<From, To>::value>::type;

        template <typename A, typename B>
        struct is_safe_floating_point_comparision
            : std::integral_constant<bool, is_safe_floating_point_conversion<A, B>::value
                                               || is_safe_floating_point_conversion<B, A>::value>
        {
        };

        template <typename A, typename B>
        using enable_safe_floating_point_comparision =
            typename std::enable_if<is_safe_floating_point_comparision<A, B>::value>::type;

        template <typename A, typename B>
        using fallback_safe_floating_point_comparision =
            typename std::enable_if<!is_safe_floating_point_comparision<A, B>::value>::type;

        template <typename A, typename B>
        struct is_safe_floating_point_operation
            : std::integral_constant<bool, std::is_floating_point<A>::value
                                               && std::is_floating_point<B>::value>
        {
        };

        template <typename A, typename B>
        using floating_point_result_t =
            typename std::enable_if<is_safe_floating_point_operation<A, B>::value,
                                    typename std::conditional<sizeof(A) < sizeof(B), B,
                                                              A>::type>::type;
        template <typename A, typename B>
        using fallback_floating_point_result =
            typename std::enable_if<!is_safe_floating_point_operation<A, B>::value>::type;
    } // namespace detail

    /// A type safe floating point class.
    ///
    /// It is a tiny, no overhead wrapper over a standard floating point type.
    /// It behaves exactly like the built-in types except it does not allow narrowing conversions.
    ///
    /// \requires `FloatT` must be a floating point type.
    /// \notes It intentionally does not provide equality or increment/decrement operators.
    template <typename FloatT>
    class floating_point
    {
        static_assert(std::is_floating_point<FloatT>::value, "must be a floating point type");

    public:
        using floating_point_type = FloatT;

        //=== constructors ===//
        floating_point() = delete;

        template <typename T,
                  typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
        TYPE_SAFE_FORCE_INLINE constexpr floating_point(const T& val) noexcept : value_(val)
        {
        }

        template <typename T,
                  typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
        TYPE_SAFE_FORCE_INLINE constexpr floating_point(const floating_point<T>& val) noexcept
            : value_(static_cast<T>(val))
        {
        }

        template <
            typename T,
            typename = detail::fallback_safe_floating_point_conversion<T, floating_point_type>>
        constexpr floating_point(T) = delete;

        //=== assignment ===//
        template <typename T,
                  typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
        TYPE_SAFE_FORCE_INLINE floating_point& operator=(const T& val) noexcept
        {
            value_ = val;
            return *this;
        }

        template <typename T,
                  typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
        TYPE_SAFE_FORCE_INLINE floating_point& operator=(const floating_point<T>& val) noexcept
        {
            value_ = static_cast<T>(val);
            return *this;
        }

        template <
            typename T,
            typename = detail::fallback_safe_floating_point_conversion<T, floating_point_type>>
        floating_point& operator=(T) = delete;

        //=== conversion back ===//
        TYPE_SAFE_FORCE_INLINE explicit constexpr operator floating_point_type() const noexcept
        {
            return value_;
        }

        //=== unary operators ===//
        TYPE_SAFE_FORCE_INLINE constexpr floating_point operator+() const noexcept
        {
            return *this;
        }

        TYPE_SAFE_FORCE_INLINE constexpr floating_point operator-() const noexcept
        {
            return -value_;
        }

//=== compound assignment ====//
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <typename T,                                                                          \
              typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>    \
    TYPE_SAFE_FORCE_INLINE floating_point& operator Op(const T& other) noexcept                    \
    {                                                                                              \
        return *this Op floating_point<T>(other);                                                  \
    }                                                                                              \
    template <typename T,                                                                          \
              typename = detail::fallback_safe_floating_point_conversion<T, floating_point_type>>  \
    floating_point& operator Op(floating_point<T>) = delete;                                       \
    template <typename T,                                                                          \
              typename = detail::fallback_safe_floating_point_conversion<T, floating_point_type>>  \
    floating_point& operator Op(T) = delete;

        template <typename T,
                  typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
        TYPE_SAFE_FORCE_INLINE floating_point& operator+=(const floating_point<T>& other) noexcept
        {
            value_ += static_cast<T>(other);
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(+=)

        template <typename T,
                  typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
        TYPE_SAFE_FORCE_INLINE floating_point& operator-=(const floating_point<T>& other) noexcept
        {
            value_ -= static_cast<T>(other);
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(-=)

        template <typename T,
                  typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
        TYPE_SAFE_FORCE_INLINE floating_point& operator*=(const floating_point<T>& other) noexcept
        {
            value_ *= static_cast<T>(other);
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(*=)

        template <typename T,
                  typename = detail::enable_safe_floating_point_conversion<T, floating_point_type>>
        TYPE_SAFE_FORCE_INLINE floating_point& operator/=(const floating_point<T>& other) noexcept
        {
            value_ /= static_cast<T>(other);
            return *this;
        }
        TYPE_SAFE_DETAIL_MAKE_OP(/=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

    private:
        floating_point_type value_;
    };

//=== comparision ===//
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <typename A, typename B,                                                              \
              typename = detail::enable_safe_floating_point_conversion<A, B>>                      \
    TYPE_SAFE_FORCE_INLINE constexpr bool operator Op(const A& a, const floating_point<B>& b)      \
    {                                                                                              \
        return floating_point<A>(a) Op b;                                                          \
    }                                                                                              \
    template <typename A, typename B,                                                              \
              typename = detail::enable_safe_floating_point_conversion<A, B>>                      \
    TYPE_SAFE_FORCE_INLINE constexpr bool operator Op(const floating_point<A>& a, const B& b)      \
    {                                                                                              \
        return a Op floating_point<B>(b);                                                          \
    }                                                                                              \
    template <typename A, typename B,                                                              \
              typename = detail::fallback_safe_floating_point_comparision<A, B>>                   \
    constexpr bool operator Op(floating_point<A>, floating_point<B>) = delete;                     \
    template <typename A, typename B,                                                              \
              typename = detail::fallback_safe_floating_point_comparision<A, B>>                   \
    constexpr bool operator Op(A, floating_point<B>) = delete;                                     \
    template <typename A, typename B,                                                              \
              typename = detail::fallback_safe_floating_point_comparision<A, B>>                   \
    constexpr bool operator Op(floating_point<A>, B) = delete;

    template <typename A, typename B,
              typename = detail::enable_safe_floating_point_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator<(const floating_point<A>& a,
                                                    const floating_point<B>& b) noexcept
    {
        return static_cast<A>(a) < static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(<)

    template <typename A, typename B,
              typename = detail::enable_safe_floating_point_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator<=(const floating_point<A>& a,
                                                     const floating_point<B>& b) noexcept
    {
        return static_cast<A>(a) <= static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(<=)

    template <typename A, typename B,
              typename = detail::enable_safe_floating_point_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator>(const floating_point<A>& a,
                                                    const floating_point<B>& b) noexcept
    {
        return static_cast<A>(a) > static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(>)

    template <typename A, typename B,
              typename = detail::enable_safe_floating_point_comparision<A, B>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator>=(const floating_point<A>& a,
                                                     const floating_point<B>& b) noexcept
    {
        return static_cast<A>(a) >= static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(>=)

#undef TYPE_SAFE_DETAIL_MAKE_OP

//=== binary operations ===//
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <typename A, typename B>                                                              \
    TYPE_SAFE_FORCE_INLINE constexpr auto operator Op(const A&                 a,                  \
                                                      const floating_point<B>& b) noexcept         \
        ->floating_point<detail::floating_point_result_t<A, B>>                                    \
    {                                                                                              \
        return floating_point<A>(a) Op b;                                                          \
    }                                                                                              \
    template <typename A, typename B>                                                              \
    TYPE_SAFE_FORCE_INLINE constexpr auto operator Op(const floating_point<A>& a,                  \
                                                      const B&                 b) noexcept         \
        ->floating_point<detail::floating_point_result_t<A, B>>                                    \
    {                                                                                              \
        return a Op floating_point<B>(b);                                                          \
    }                                                                                              \
    template <typename A, typename B, typename = detail::fallback_floating_point_result<A, B>>     \
    constexpr int operator Op(floating_point<A>, floating_point<B>) noexcept = delete;             \
    template <typename A, typename B, typename = detail::fallback_floating_point_result<A, B>>     \
    constexpr int operator Op(A, floating_point<B>) noexcept = delete;                             \
    template <typename A, typename B, typename = detail::fallback_floating_point_result<A, B>>     \
    constexpr int operator Op(floating_point<A>, B) noexcept = delete;

    template <typename A, typename B>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator+(const floating_point<A>& a,
                                                    const floating_point<B>& b) noexcept
        -> floating_point<detail::floating_point_result_t<A, B>>
    {
        return static_cast<A>(a) + static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(+)

    template <typename A, typename B>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator-(const floating_point<A>& a,
                                                    const floating_point<B>& b) noexcept
        -> floating_point<detail::floating_point_result_t<A, B>>
    {
        return static_cast<A>(a) - static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(-)

    template <typename A, typename B>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator*(const floating_point<A>& a,
                                                    const floating_point<B>& b) noexcept
        -> floating_point<detail::floating_point_result_t<A, B>>
    {
        return static_cast<A>(a) * static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(*)

    template <typename A, typename B>
    TYPE_SAFE_FORCE_INLINE constexpr auto operator/(const floating_point<A>& a,
                                                    const floating_point<B>& b) noexcept
        -> floating_point<detail::floating_point_result_t<A, B>>
    {
        return static_cast<A>(a) / static_cast<B>(b);
    }
    TYPE_SAFE_DETAIL_MAKE_OP(/)

#undef TYPE_SAFE_DETAIL_MAKE_OP

    //=== input/output ===/
    template <typename Char, class CharTraits, typename FloatT>
    std::basic_istream<Char, CharTraits>& operator>>(std::basic_istream<Char, CharTraits>& in,
                                                     floating_point<FloatT>& f)
    {
        FloatT val;
        in >> val;
        f = val;
        return in;
    }

    template <typename Char, class CharTraits, typename FloatT>
    std::basic_ostream<Char, CharTraits>& operator<<(std::basic_ostream<Char, CharTraits>& out,
                                                     const floating_point<FloatT>& f)
    {
        return out << static_cast<FloatT>(f);
    }
} // namespace type_safe

#endif // TYPE_SAFE_FLOATING_POINT_HPP_INCLUDED
