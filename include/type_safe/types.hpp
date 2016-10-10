// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_TYPES_HPP_INCLUDED
#define TYPE_SAFE_TYPES_HPP_INCLUDED

#include <cstddef>
#include <cstdint>
#include <cmath>

#include <type_safe/boolean.hpp>
#include <type_safe/floating_point.hpp>
#include <type_safe/integer.hpp>

#ifndef TYPE_SAFE_ENABLE_WRAPPER
#define TYPE_SAFE_ENABLE_WRAPPER 1
#endif

namespace type_safe
{
#if TYPE_SAFE_DISABLE_WRAPPER
#define TYPE_SAFE_DETAIL_WRAP(templ, x) x
#else
/// \exclude
#define TYPE_SAFE_DETAIL_WRAP(templ, x) templ<x>
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

    //=== floating point types ===//
    using float_t  = TYPE_SAFE_DETAIL_WRAP(floating_point, std::float_t);
    using double_t = TYPE_SAFE_DETAIL_WRAP(floating_point, std::double_t);

//=== boolean ===//
#if TYPE_SAFE_DISABLE_WRAPPER
    using bool_t = bool;
#else
    using bool_t = boolean;
#endif

#undef TYPE_SAFE_DETAIL_WRAP
} // namespace type_safe

#endif // TYPE_SAFE_TYPES_HPP_INCLUDED
