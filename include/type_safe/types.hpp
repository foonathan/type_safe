// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_TYPES_HPP_INCLUDED
#define TYPE_SAFE_TYPES_HPP_INCLUDED

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>

#include <type_safe/boolean.hpp>
#include <type_safe/config.hpp>
#include <type_safe/floating_point.hpp>
#include <type_safe/integer.hpp>

namespace type_safe
{
    /// \exclude
    namespace detail
    {
        template <typename... Ts>
        struct conditional_impl;

        template <typename Else>
        struct conditional_impl<Else>
        {
            using type = Else;
        };

        template <typename Result, typename... Tail>
        struct conditional_impl<std::true_type, Result, Tail...>
        {
            using type = Result;
        };

        template <typename Result, typename... Tail>
        struct conditional_impl<std::false_type, Result, Tail...>
        {
            using type = typename conditional_impl<Tail...>::type;
        };

        template <typename... Ts>
        using conditional = typename conditional_impl<Ts...>::type;

        template <bool Value>
        using bool_constant = std::integral_constant<bool, Value>;

        struct decimal_digit
        {
        };
        struct lower_hexadecimal_digit
        {
        };
        struct upper_hexadecimal_digit
        {
        };
        struct no_digit
        {
        };

        template <char C>
        using digit_category =
            conditional<bool_constant<C >= '0' && C <= '9'>, decimal_digit,
                        bool_constant<C >= 'a' && C <= 'f'>, lower_hexadecimal_digit,
                        bool_constant<C >= 'A' && C <= 'F'>, upper_hexadecimal_digit, no_digit>;

        template <char C, typename Cat>
        struct to_digit_impl
        {
            static_assert(!std::is_same<Cat, no_digit>::value, "invalid character, expected digit");
        };

        template <char C>
        struct to_digit_impl<C, decimal_digit>
        {
            static constexpr auto value = static_cast<int>(C) - static_cast<int>('0');
        };

        template <char C>
        struct to_digit_impl<C, lower_hexadecimal_digit>
        {
            static constexpr auto value = static_cast<int>(C) - static_cast<int>('a') + 10;
        };

        template <char C>
        struct to_digit_impl<C, upper_hexadecimal_digit>
        {
            static constexpr auto value = static_cast<int>(C) - static_cast<int>('A') + 10;
        };

        template <typename T, T Base, char C>
        constexpr T to_digit()
        {
            using impl = to_digit_impl<C, digit_category<C>>;
            static_assert(impl::value < Base, "invalid digit for base");
            return impl::value;
        }

        template <char... Digits>
        struct parse_loop;

        template <>
        struct parse_loop<>
        {
            template <typename T, T>
            static constexpr T parse(T value)
            {
                return value;
            }
        };

        template <char... Tail>
        struct parse_loop<'\'', Tail...>
        {
            template <typename T, T Base>
            static constexpr T parse(T value)
            {
                return parse_loop<Tail...>::template parse<T, Base>(value);
            }
        };

        template <char Head, char... Tail>
        struct parse_loop<Head, Tail...>
        {
            template <typename T, T Base>
            static constexpr T parse(T value)
            {
                return parse_loop<Tail...>::template parse<T, Base>(value * Base
                                                                    + to_digit<T, Base, Head>());
            }
        };

        template <typename T, T Base, char Head, char... Tail>
        constexpr T do_parse_loop()
        {
            return parse_loop<Tail...>::template parse<T, Base>(to_digit<T, Base, Head>());
        }

        template <typename T, char... Digits>
        struct parse_base
        {
            static constexpr T parse()
            {
                return do_parse_loop<T, 10, Digits...>();
            }
        };

        template <typename T, char... Tail>
        struct parse_base<T, '0', Tail...>
        {
            static constexpr T parse()
            {
                return do_parse_loop<T, 8, Tail...>();
            }
        };

        template <typename T, char... Tail>
        struct parse_base<T, '0', 'x', Tail...>
        {
            static constexpr T parse()
            {
                return do_parse_loop<T, 16, Tail...>();
            }
        };

        template <typename T, char... Tail>
        struct parse_base<T, '0', 'X', Tail...>
        {
            static constexpr T parse()
            {
                return do_parse_loop<T, 16, Tail...>();
            }
        };

        template <typename T, char... Tail>
        struct parse_base<T, '0', 'b', Tail...>
        {
            static constexpr T parse()
            {
                return do_parse_loop<T, 2, Tail...>();
            }
        };

        template <typename T, char... Tail>
        struct parse_base<T, '0', 'B', Tail...>
        {
            static constexpr T parse()
            {
                return do_parse_loop<T, 2, Tail...>();
            }
        };

        template <typename T, char... Digits>
        constexpr T parse()
        {
            return parse_base<T, Digits...>::parse();
        }

        template <typename T, typename U, U Value>
        constexpr T validate_value()
        {
            static_assert(sizeof(T) <= sizeof(U)
                              && std::is_signed<U>::value == std::is_signed<T>::value,
                          "mismatched types");
            static_assert(U(std::numeric_limits<T>::min()) <= Value
                              && Value <= U(std::numeric_limits<T>::max()),
                          "integer literal overflow");
            return static_cast<T>(Value);
        }

        template <typename T, char... Digits>
        constexpr T parse_signed()
        {
            return validate_value<T, long long, parse<long long, Digits...>()>();
        }

        template <typename T, char... Digits>
        constexpr T parse_unsigned()
        {
            return validate_value<T, unsigned long long, parse<unsigned long long, Digits...>()>();
        }
    } // namespace detail

#if TYPE_SAFE_ENABLE_WRAPPER
/// \exclude
#define TYPE_SAFE_DETAIL_WRAP(templ, x) templ<x>
#else
#define TYPE_SAFE_DETAIL_WRAP(templ, x) x
#endif

    //=== fixed with integer ===//
    /// \module types
    using int8_t = TYPE_SAFE_DETAIL_WRAP(integer, int8_t);
    /// \module types
    using int16_t = TYPE_SAFE_DETAIL_WRAP(integer, int16_t);
    /// \module types
    using int32_t = TYPE_SAFE_DETAIL_WRAP(integer, int32_t);
    /// \module types
    using int64_t = TYPE_SAFE_DETAIL_WRAP(integer, int64_t);
    /// \module types
    using uint8_t = TYPE_SAFE_DETAIL_WRAP(integer, uint8_t);
    /// \module types
    using uint16_t = TYPE_SAFE_DETAIL_WRAP(integer, uint16_t);
    /// \module types
    using uint32_t = TYPE_SAFE_DETAIL_WRAP(integer, uint32_t);
    /// \module types
    using uint64_t = TYPE_SAFE_DETAIL_WRAP(integer, uint64_t);

    inline namespace literals
    {
        /// \module types
        template <char... Digits>
        constexpr int8_t operator"" _i8()
        {
            return int8_t(detail::parse_signed<std::int8_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr int16_t operator"" _i16()
        {
            return int16_t(detail::parse_signed<std::int16_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr int32_t operator"" _i32()
        {
            return int32_t(detail::parse_signed<std::int32_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr int64_t operator"" _i64()
        {
            return int64_t(detail::parse_signed<std::int64_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr uint8_t operator"" _u8()
        {
            return uint8_t(detail::parse_unsigned<std::uint8_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr uint16_t operator"" _u16()
        {
            return uint16_t(detail::parse_unsigned<std::uint16_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr uint32_t operator"" _u32()
        {
            return uint32_t(detail::parse_unsigned<std::uint32_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr uint64_t operator"" _u64()
        {
            return uint64_t(detail::parse_unsigned<std::uint64_t, Digits...>());
        }
    } // namespace literals

    /// \module types
    using int_fast8_t = TYPE_SAFE_DETAIL_WRAP(integer, int_fast8_t);
    /// \module types
    using int_fast16_t = TYPE_SAFE_DETAIL_WRAP(integer, int_fast16_t);
    /// \module types
    using int_fast32_t = TYPE_SAFE_DETAIL_WRAP(integer, int_fast32_t);
    /// \module types
    using int_fast64_t = TYPE_SAFE_DETAIL_WRAP(integer, int_fast64_t);
    /// \module types
    using uint_fast8_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_fast8_t);
    /// \module types
    using uint_fast16_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_fast16_t);
    /// \module types
    using uint_fast32_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_fast32_t);
    /// \module types
    using uint_fast64_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_fast64_t);

    /// \module types
    using int_least8_t = TYPE_SAFE_DETAIL_WRAP(integer, int_least8_t);
    /// \module types
    using int_least16_t = TYPE_SAFE_DETAIL_WRAP(integer, int_least16_t);
    /// \module types
    using int_least32_t = TYPE_SAFE_DETAIL_WRAP(integer, int_least32_t);
    /// \module types
    using int_least64_t = TYPE_SAFE_DETAIL_WRAP(integer, int_least64_t);
    /// \module types
    using uint_least8_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_least8_t);
    /// \module types
    using uint_least16_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_least16_t);
    /// \module types
    using uint_least32_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_least32_t);
    /// \module types
    using uint_least64_t = TYPE_SAFE_DETAIL_WRAP(integer, uint_least64_t);

    /// \module types
    using intmax_t = TYPE_SAFE_DETAIL_WRAP(integer, intmax_t);
    /// \module types
    using uintmax_t = TYPE_SAFE_DETAIL_WRAP(integer, uintmax_t);
    /// \module types
    using intptr_t = TYPE_SAFE_DETAIL_WRAP(integer, intptr_t);
    /// \module types
    using uintptr_t = TYPE_SAFE_DETAIL_WRAP(integer, uintptr_t);

    //=== special integer types ===//
    /// \module types
    using ptrdiff_t = TYPE_SAFE_DETAIL_WRAP(integer, ptrdiff_t);
    /// \module types
    using size_t = TYPE_SAFE_DETAIL_WRAP(integer, size_t);

    /// \module types
    using int_t = TYPE_SAFE_DETAIL_WRAP(integer, int);
    /// \module types
    using unsigned_t = TYPE_SAFE_DETAIL_WRAP(integer, unsigned);

    inline namespace literals
    {
        /// \module types
        template <char... Digits>
        constexpr ptrdiff_t operator"" _isize()
        {
            return ptrdiff_t(detail::parse_signed<std::ptrdiff_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr size_t operator"" _usize()
        {
            return size_t(detail::parse_unsigned<std::size_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr int_t operator"" _i()
        {
            // int is at least 16 bits
            return int_t(detail::parse_signed<std::int16_t, Digits...>());
        }

        /// \module types
        template <char... Digits>
        constexpr unsigned_t operator"" _u()
        {
            // int is at least 16 bits
            return unsigned_t(detail::parse_unsigned<std::uint16_t, Digits...>());
        }
    } // namespace literalse

    //=== floating point types ===//
    /// \module types
    using float_t = TYPE_SAFE_DETAIL_WRAP(floating_point, std::float_t);
    /// \module types
    using double_t = TYPE_SAFE_DETAIL_WRAP(floating_point, std::double_t);

    inline namespace literals
    {
        /// \module types
        constexpr float_t operator"" _f(long double val)
        {
            return float_t(static_cast<std::float_t>(val));
        }

        /// \module types
        constexpr double_t operator"" _d(long double val)
        {
            return double_t(static_cast<std::double_t>(val));
        }
    }

//=== boolean ===//
#if TYPE_SAFE_ENABLE_WRAPPER
    /// \module types
    using bool_t = boolean;
#else
    using bool_t = bool;
#endif

#undef TYPE_SAFE_DETAIL_WRAP
} // namespace type_safe

#endif // TYPE_SAFE_TYPES_HPP_INCLUDED
