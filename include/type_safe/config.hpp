// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_CONFIG_HPP_INCLUDED
#define TYPE_SAFE_CONFIG_HPP_INCLUDED

#include <cstddef>

#ifndef TYPE_SAFE_ENABLE_ASSERTIONS
#define TYPE_SAFE_ENABLE_ASSERTIONS 1
#endif

#ifndef TYPE_SAFE_ENABLE_WRAPPER
#define TYPE_SAFE_ENABLE_WRAPPER 1
#endif

#ifndef TYPE_SAFE_ARITHMETIC_UB
#define TYPE_SAFE_ARITHMETIC_UB 1
#endif

#ifndef TYPE_SAFE_USE_REF_QUALIFIERS
#if defined(__GNUC__)
// GCC 4.8's implementation of ref qualifiers is buggy
#define TYPE_SAFE_USE_REF_QUALIFIERS ((__GNUC__ >= 5) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9))
#elif defined(__cpp_ref_qualifiers) && __cpp_ref_qualifiers >= 200710
#define TYPE_SAFE_USE_REF_QUALIFIERS 1
#else
#define TYPE_SAFE_USE_REF_QUALIFIERS 0
#endif
#endif

#if TYPE_SAFE_USE_REF_QUALIFIERS
#define TYPE_SAFE_LVALUE_REF &
#define TYPE_SAFE_RVALUE_REF &&
#else
#define TYPE_SAFE_LVALUE_REF
#define TYPE_SAFE_RVALUE_REF
#endif

#endif // TYPE_SAFE_CONFIG_HPP_INCLUDED
