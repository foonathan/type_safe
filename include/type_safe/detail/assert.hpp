// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_DETAIL_ASSERT_HPP_INCLUDED
#define TYPE_SAFE_DETAIL_ASSERT_HPP_INCLUDED

#include <debug_assert.hpp>

#ifndef TYPE_SAFE_ENABLE_ASSERTIONS
#define TYPE_SAFE_ENABLE_ASSERTIONS 1
#endif

namespace type_safe
{
    namespace detail
    {
        struct assert_handler : debug_assert::set_level<TYPE_SAFE_ENABLE_ASSERTIONS>,
                                debug_assert::default_handler
        {
        };
    } // namespace detail
} // namespace type_safe

#endif // TYPE_SAFE_DETAIL_ASSERT_HPP_INCLUDED`
