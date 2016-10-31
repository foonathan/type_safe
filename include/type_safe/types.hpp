// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_TYPES_HPP_INCLUDED
#define TYPE_SAFE_TYPES_HPP_INCLUDED

#include <cstddef>
#include <cstdint>
#include <cmath>

#include <type_safe/boolean.hpp>
#include <type_safe/config.hpp>
#include <type_safe/floating_point.hpp>
#include <type_safe/integer.hpp>

namespace type_safe
{
#if TYPE_SAFE_ENABLE_WRAPPER
/// \exclude
#define TYPE_SAFE_DETAIL_WRAP(templ, x) templ<x>
#else
#define TYPE_SAFE_DETAIL_WRAP(templ, x) x
#endif

    //=== fixed with integer ===//
    using int8_t   = TYPE_SAFE_DETAIL_WRAP(integer, int8_t);
    using int16_t  = TYPE_SAFE_DETAIL_WRAP(integer, int16_t);
    using int32_t  = TYPE_SAFE_DETAIL_WRAP(integer, int32_t);
    using int64_t  = TYPE_SAFE_DETAIL_WRAP(integer, int64_t);
    using uint8_t  = TYPE_SAFE_DETAIL_WRAP(integer, uint8_t);
    using uint16_t = TYPE_SAFE_DETAIL_WRAP(integer, uint16_t);
    using uint32_t = TYPE_SAFE_DETAIL_WRAP(integer, uint32_t);
    using uint64_t = TYPE_SAFE_DETAIL_WRAP(integer, uint64_t);

    inline namespace literals
    {
        constexpr int8_t operator""_i8(unsigned long long val)
        {
            return int8_t(static_cast<std::int8_t>(val));
        }

        constexpr int16_t operator""_i16(unsigned long long val)
        {
            return int16_t(static_cast<std::int16_t>(val));
        }

        constexpr int32_t operator""_i32(unsigned long long val)
        {
            return int32_t(static_cast<std::int32_t>(val));
        }

        constexpr int64_t operator""_i64(unsigned long long val)
        {
            return int64_t(static_cast<std::int64_t>(val));
        }

        constexpr uint8_t operator""_u8(unsigned long long val)
        {
            return uint8_t(static_cast<std::uint8_t>(val));
        }

        constexpr uint16_t operator""_u16(unsigned long long val)
        {
            return uint16_t(static_cast<std::uint16_t>(val));
        }

        constexpr uint32_t operator""_u32(unsigned long long val)
        {
            return uint32_t(static_cast<std::uint32_t>(val));
        }

        constexpr uint64_t operator""_u64(unsigned long long val)
        {
            return uint64_t(static_cast<std::uint64_t>(val));
        }
    } // namespace literals

    using int_fast8_t   = TYPE_SAFE_DETAIL_WRAP(integer, int_fast8_t);
    using int_fast16_t  = TYPE_SAFE_DETAIL_WRAP(integer, int_fast16_t);
    using int_fast32_t  = TYPE_SAFE_DETAIL_WRAP(integer, int_fast32_t);
    using int_fast64_t  = TYPE_SAFE_DETAIL_WRAP(integer, int_fast64_t);
    using uint_fast8_t  = TYPE_SAFE_DETAIL_WRAP(integer, uint_fast8_t);
    using uint_fast16_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_fast16_t);
    using uint_fast32_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_fast32_t);
    using uint_fast64_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_fast64_t);

    using int_least8_t   = TYPE_SAFE_DETAIL_WRAP(integer, int_least8_t);
    using int_least16_t  = TYPE_SAFE_DETAIL_WRAP(integer, int_least16_t);
    using int_least32_t  = TYPE_SAFE_DETAIL_WRAP(integer, int_least32_t);
    using int_least64_t  = TYPE_SAFE_DETAIL_WRAP(integer, int_least64_t);
    using uint_least8_t  = TYPE_SAFE_DETAIL_WRAP(integer, uint_least8_t);
    using uint_least16_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_least16_t);
    using uint_least32_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_least32_t);
    using uint_least64_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_least64_t);

    using intmax_t  = TYPE_SAFE_DETAIL_WRAP(integer, intmax_t);
    using uintmax_t = TYPE_SAFE_DETAIL_WRAP(integer, uintmax_t);
    using intptr_t  = TYPE_SAFE_DETAIL_WRAP(integer, intptr_t);
    using uintptr_t = TYPE_SAFE_DETAIL_WRAP(integer, uintptr_t);

    //=== special integer types ===//
    using ptrdiff_t = TYPE_SAFE_DETAIL_WRAP(integer, ptrdiff_t);
    using size_t    = TYPE_SAFE_DETAIL_WRAP(integer, size_t);

    using int_t      = TYPE_SAFE_DETAIL_WRAP(integer, int);
    using unsigned_t = TYPE_SAFE_DETAIL_WRAP(integer, unsigned);

    inline namespace literals
    {
        constexpr ptrdiff_t operator""_isize(unsigned long long val)
        {
            return ptrdiff_t(static_cast<std::ptrdiff_t>(val));
        }

        constexpr size_t operator""_usize(unsigned long long val)
        {
            return size_t(static_cast<std::size_t>(val));
        }

        constexpr int_t operator""_i(unsigned long long val)
        {
            return int_t(static_cast<int>(val));
        }

        constexpr unsigned_t operator""_u(unsigned long long val)
        {
            return unsigned_t(static_cast<unsigned>(val));
        }
    } // namespace literalse

    //=== floating point types ===//
    using float_t  = TYPE_SAFE_DETAIL_WRAP(floating_point, std::float_t);
    using double_t = TYPE_SAFE_DETAIL_WRAP(floating_point, std::double_t);

    inline namespace literals
    {
        constexpr float_t operator""_f(long double val)
        {
            return float_t(static_cast<std::float_t>(val));
        }

        constexpr double_t operator""_d(long double val)
        {
            return double_t(static_cast<std::double_t>(val));
        }
    }

//=== boolean ===//
#if TYPE_SAFE_ENABLE_WRAPPER
    using bool_t = boolean;
#else
    using bool_t = bool;
#endif

#undef TYPE_SAFE_DETAIL_WRAP
} // namespace type_safe

#endif // TYPE_SAFE_TYPES_HPP_INCLUDED
