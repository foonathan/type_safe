// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_CONFIG_HPP_INCLUDED
#define TYPE_SAFE_CONFIG_HPP_INCLUDED

#include <cstddef>
#include <cstdlib>

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
#if defined(__cpp_ref_qualifiers) && __cpp_ref_qualifiers >= 200710
#define TYPE_SAFE_USE_REF_QUALIFIERS 1
#elif defined(_MSC_VER) && _MSC_VER >= 1900
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

#ifndef TYPE_SAFE_USE_NOEXCEPT_DEFAULT

#if defined(__GNUC__) && __GNUC__ < 5
// GCC before 5.0 doesn't handle noexcept and = default properly
#define TYPE_SAFE_USE_NOEXCEPT_DEFAULT 0
#else
#define TYPE_SAFE_USE_NOEXCEPT_DEFAULT 1
#endif

#endif

#if TYPE_SAFE_USE_NOEXCEPT_DEFAULT
#define TYPE_SAFE_NOEXCEPT_DEFAULT(Val) noexcept(Val)
#else
#define TYPE_SAFE_NOEXCEPT_DEFAULT(Val)
#endif

#ifndef TYPE_SAFE_USE_EXCEPTIONS

#if __cpp_exceptions
#define TYPE_SAFE_USE_EXCEPTIONS 1
#elif defined(__GNUC__) && defined(__EXCEPTIONS)
#define TYPE_SAFE_USE_EXCEPTIONS 1
#elif defined(_MSC_VER) && defined(_CPPUNWIND)
#define TYPE_SAFE_USE_EXCEPTIONS 1
#else
#define TYPE_SAFE_USE_EXCEPTIONS 0
#endif

#endif

#if TYPE_SAFE_USE_EXCEPTIONS
#define TYPE_SAFE_THROW(Ex) throw Ex
#define TYPE_SAFE_TRY try
#define TYPE_SAFE_CATCH_ALL catch (...)
#define TYPE_SAFE_RETHROW throw
#else
#define TYPE_SAFE_THROW(Ex) (Ex, std::abort())
#define TYPE_SAFE_TRY if (true)
#define TYPE_SAFE_CATCH_ALL if (false)
#define TYPE_SAFE_RETHROW std::abort()
#endif

/// \entity type_safe
/// \unique_name ts

#endif // TYPE_SAFE_CONFIG_HPP_INCLUDED
