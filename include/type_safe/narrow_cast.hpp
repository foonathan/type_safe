// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_NARROW_CAST_HPP_INCLUDED
#define TYPE_SAFE_NARROW_CAST_HPP_INCLUDED

#include <type_safe/floating_point.hpp>
#include <type_safe/integer.hpp>

namespace type_safe
{
    namespace detail
    {
        template <typename T>
        struct get_target_integer
        {
            using type = integer<T>;
        };

        template <typename T>
        struct get_target_integer<integer<T>>
        {
            using type = integer<T>;
        };

        template <typename T>
        struct get_target_floating_point
        {
            using type = floating_point<T>;
        };

        template <typename T>
        struct get_target_floating_point<floating_point<T>>
        {
            using type = floating_point<T>;
        };

        template <typename Target, typename Source>
        TYPE_SAFE_FORCE_INLINE constexpr bool is_narrowing(const integer<Source>& source)
        {
            using target_t = typename get_target_integer<Target>::type::integer_type;
            using limits   = std::numeric_limits<target_t>;
            return sizeof(target_t) < sizeof(Source) // no narrowing possible
                   && (source > Source(limits::max())
                       || source < Source(limits::min())); // otherwise check bounds
        }

        template <typename Target, typename Source>
        TYPE_SAFE_FORCE_INLINE constexpr bool is_narrowing(const floating_point<Source>& source)
        {
            using target_t = typename get_target_floating_point<Target>::type::floating_point_type;
            return sizeof(target_t) < sizeof(Source) // no narrowing possible
                   // cast source -> underlying float -> target float -> source
                   // and check if it changed the value
                   && static_cast<Source>(static_cast<target_t>(static_cast<Source>(source)))
                          != static_cast<Source>(source);
        }
    } // namespace detail

    /// \returns A [type_safe::integer]() with the same value as `source` but of a different type.
    /// \requires The value of `source` must be representable by the new target type.
    /// \notes `Target` can either be a specialization of the `integer` template itself
    /// or a built-in integer type, the result will be wrapped if needed.
    template <typename Target, typename Source>
    TYPE_SAFE_FORCE_INLINE constexpr auto narrow_cast(const integer<Source>& source) noexcept ->
        typename detail::get_target_integer<Target>::type
    {
        using target_integer = typename detail::get_target_integer<Target>::type;
        using target_t       = typename target_integer::integer_type;
        return detail::is_narrowing<Target>(source) ?
                   (DEBUG_UNREACHABLE(detail::assert_handler{}, "conversion would truncate value"),
                    target_t()) :
                   target_integer(static_cast<target_t>(static_cast<Source>(source)));
    }

    /// \returns A [type_safe::floating_point]() with the same value as `source` but of a different type.
    /// \requires The value of `source` must be representable by the new target type.
    /// \notes `Target` can either be a specialization of the `floating_point` template itself
    /// or a built-in floating point type, the result will be wrapped if needed.
    template <typename Target, typename Source>
    TYPE_SAFE_FORCE_INLINE constexpr auto narrow_cast(const floating_point<Source>& source) noexcept
        -> typename detail::get_target_floating_point<Target>::type
    {
        using target_float = typename detail::get_target_floating_point<Target>::type;
        using target_t     = typename target_float::floating_point_type;
        return detail::is_narrowing<Target>(source) ?
                   (DEBUG_UNREACHABLE(detail::assert_handler{}, "conversion would truncate value"),
                    target_t()) :
                   target_float(static_cast<target_t>(static_cast<Source>(source)));
    }
} // namespace type_safe

#endif // TYPE_SAFE_NARROW_CAST_HPP_INCLUDED
