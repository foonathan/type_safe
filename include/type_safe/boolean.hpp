// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_BOOLEAN_HPP_INCLUDED
#define TYPE_SAFE_BOOLEAN_HPP_INCLUDED

#include <type_safe/detail/force_inline.hpp>

namespace type_safe
{
    /// A type safe boolean class.
    ///
    /// It can only be constructed/assigned `bool` values
    /// and does not provide all the operations.
    class boolean
    {
    public:
        TYPE_SAFE_FORCE_INLINE constexpr boolean(bool value) noexcept : value_(value)
        {
        }

        template <typename T>
        constexpr boolean(const T&) = delete;

        TYPE_SAFE_FORCE_INLINE boolean& operator=(bool value) noexcept
        {
            value_ = value;
            return *this;
        }

        template <typename T>
        boolean& operator=(const T&) = delete;

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

    TYPE_SAFE_FORCE_INLINE constexpr bool operator==(const boolean& a, bool b) noexcept
    {
        return static_cast<bool>(a) == b;
    }

    TYPE_SAFE_FORCE_INLINE constexpr bool operator==(bool a, const boolean& b) noexcept
    {
        return a == static_cast<bool>(b);
    }

    template <typename T>
    constexpr bool operator==(const boolean&, const T&) = delete;

    template <typename T>
    constexpr bool operator==(const T&, const boolean&) = delete;

    TYPE_SAFE_FORCE_INLINE constexpr bool operator!=(const boolean& a, const boolean& b) noexcept
    {
        return !(a == b);
    }

    TYPE_SAFE_FORCE_INLINE constexpr bool operator!=(const boolean& a, bool b) noexcept
    {
        return !(a == b);
    }

    TYPE_SAFE_FORCE_INLINE constexpr bool operator!=(bool a, const boolean& b) noexcept
    {
        return !(a == b);
    }

    template <typename T>
    constexpr bool operator!=(const boolean&, const T&) = delete;

    template <typename T>
    constexpr bool operator!=(const T&, const boolean&) = delete;
} // namespace type_safe

#endif // TYPE_SAFE_BOOLEAN_HPP_INCLUDED
