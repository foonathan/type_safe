// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_BOOLEAN_HPP_INCLUDED
#define TYPE_SAFE_BOOLEAN_HPP_INCLUDED

#include <iosfwd>
#include <type_traits>

#include <type_safe/detail/force_inline.hpp>

namespace type_safe
{
    class boolean;

    /// \exclude
    namespace detail
    {
        template <typename T>
        struct is_boolean : std::false_type
        {
        };

        template <>
        struct is_boolean<bool> : std::true_type
        {
        };

        template <>
        struct is_boolean<boolean> : std::true_type
        {
        };

        template <typename T>
        using enable_boolean = typename std::enable_if<is_boolean<T>::value>::type;
    } // namespace detail

    /// A type safe boolean class.
    ///
    /// It is a tiny, no overhead wrapper over `bool`.
    /// It can only be constructed from `bool` values
    /// and does not implictly convert to integral types.
    class boolean
    {
    public:
        boolean() = delete;

        template <typename T, typename = detail::enable_boolean<T>>
        TYPE_SAFE_FORCE_INLINE constexpr boolean(T value) noexcept : value_(value)
        {
        }

        template <typename T, typename = detail::enable_boolean<T>>
        TYPE_SAFE_FORCE_INLINE boolean& operator=(T value) noexcept
        {
            value_ = value;
            return *this;
        }

        TYPE_SAFE_FORCE_INLINE explicit constexpr operator bool() const noexcept
        {
            return value_;
        }

        TYPE_SAFE_FORCE_INLINE constexpr boolean operator!() const noexcept
        {
            return boolean(!value_);
        }

    private:
        bool value_;
    };

    //=== comparision ===//
    TYPE_SAFE_FORCE_INLINE constexpr bool operator==(const boolean& a, const boolean& b) noexcept
    {
        return static_cast<bool>(a) == static_cast<bool>(b);
    }

    template <typename T, typename = detail::enable_boolean<T>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator==(const boolean& a, T b) noexcept
    {
        return static_cast<bool>(a) == static_cast<bool>(b);
    }

    template <typename T, typename = detail::enable_boolean<T>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator==(T a, const boolean& b) noexcept
    {
        return static_cast<bool>(a) == static_cast<bool>(b);
    }

    TYPE_SAFE_FORCE_INLINE constexpr bool operator!=(const boolean& a, const boolean& b) noexcept
    {
        return static_cast<bool>(a) != static_cast<bool>(b);
    }

    template <typename T, typename = detail::enable_boolean<T>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator!=(const boolean& a, T b) noexcept
    {
        return static_cast<bool>(a) != static_cast<bool>(b);
    }

    template <typename T, typename = detail::enable_boolean<T>>
    TYPE_SAFE_FORCE_INLINE constexpr bool operator!=(T a, const boolean& b) noexcept
    {
        return static_cast<bool>(a) != static_cast<bool>(b);
    }

    //=== input/output ===/
    template <typename Char, class CharTraits>
    std::basic_istream<Char, CharTraits>& operator>>(std::basic_istream<Char, CharTraits>& in,
                                                     boolean& b)
    {
        bool val;
        in >> val;
        b = val;
        return in;
    }

    template <typename Char, class CharTraits>
    std::basic_ostream<Char, CharTraits>& operator<<(std::basic_ostream<Char, CharTraits>& out,
                                                     const boolean& b)
    {
        return out << static_cast<bool>(b);
    }
} // namespace type_safe

#endif // TYPE_SAFE_BOOLEAN_HPP_INCLUDED
