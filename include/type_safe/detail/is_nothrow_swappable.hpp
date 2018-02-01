// Copyright (C) 2016-2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_DETAIL_IS_NOTHROW_SWAPABLE_HPP_INCLUDED
#define TYPE_SAFE_DETAIL_IS_NOTHROW_SWAPABLE_HPP_INCLUDED

#include <utility>

namespace type_safe
{
    namespace detail
    {
        template <typename T>
        struct is_nothrow_swappable
        {
            template <typename U>
            static auto adl_swap(int, U& a, U& b) noexcept(noexcept(swap(a, b)))
                -> decltype(swap(a, b));

            template <typename U>
            static auto adl_swap(short, U& a, U& b) noexcept(noexcept(std::swap(a, b)))
                -> decltype(std::swap(a, b));

            static void adl_swap(...) noexcept(false);

            static constexpr bool value =
                noexcept(adl_swap(0, std::declval<T&>(), std::declval<T&>()));
        };
    }
} // namespace type_safe::detail

#endif // TYPE_SAFE_DETAIL_IS_NOTHROW_SWAPABLE_HPP_INCLUDED
