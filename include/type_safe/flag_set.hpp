// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_FLAG_SET_HPP_INCLUDED
#define TYPE_SAFE_FLAG_SET_HPP_INCLUDED

#include <cstdint>
#include <climits>
#include <type_traits>

#include <type_safe/flag.hpp>
#include <type_safe/types.hpp>

namespace type_safe
{
    /// \exclude
    namespace detail
    {
        template <typename Enum, typename = void>
        struct is_flag_set : std::false_type
        {
        };

        template <typename Enum>
        struct is_flag_set<Enum, decltype(static_cast<void>(Enum::_flag_set_size))>
            : std::is_enum<Enum>
        {
        };

        template <typename Enum>
        constexpr typename std::enable_if<is_flag_set<Enum>::value, std::size_t>::type
            flag_set_size() noexcept
        {
            return static_cast<std::size_t>(Enum::_flag_set_size);
        }

        template <typename Enum>
        constexpr typename std::enable_if<!is_flag_set<Enum>::value, std::size_t>::type
            flag_set_size() noexcept
        {
            return 0u;
        }
    } // namespace detail

    /// Traits for the enum used in a [ts::flag_set]().
    ///
    /// For each enum that should be used with [ts::flag_set]() it must provide the following interface:
    /// * Inherit from [std::true_type]().
    /// * `static constexpr std::size_t size()` that returns the number of enumerators.
    ///
    /// The default specialization automatically works for enums that have an enumerator `_flag_set_size`,
    /// whose value is the number of enumerators.
    /// But you can also specialize the traits for your own enums.
    ///
    /// \requires For all specializations the enum must be contiguous starting at `0`,
    /// simply don't set an explicit value to the enumerators.
    template <typename Enum>
    struct flag_set_traits : detail::is_flag_set<Enum>
    {
        static constexpr std::size_t size() noexcept
        {
            return detail::flag_set_size<Enum>();
        }
    };

    /// \exclude
    namespace detail
    {
        template <typename Enum>
        using enable_enum_operation =
            typename std::enable_if<std::is_enum<Enum>::value
                                    && flag_set_traits<Enum>::value>::type;

        template <std::size_t Size, typename = void>
        struct select_flag_set_int
        {
            static_assert(Size != 0u,
                          "number of bits not supported, complain loud enough so I'll do it");
        };

/// \exclude
#define TYPE_SAFE_DETAIL_SELECT(Min, Max, Type)                                                    \
    template <std::size_t Size>                                                                    \
    struct select_flag_set_int<Size, typename std::enable_if<(Size > Min && Size <= Max)>::type>   \
    {                                                                                              \
        using type = Type;                                                                         \
    };

        TYPE_SAFE_DETAIL_SELECT(0u, 8u, std::uint_least8_t)
        TYPE_SAFE_DETAIL_SELECT(8u, 16u, std::uint_least16_t)
        TYPE_SAFE_DETAIL_SELECT(16u, 32u, std::uint_least32_t)
        TYPE_SAFE_DETAIL_SELECT(32u, sizeof(std::uint_least64_t) * CHAR_BIT, std::uint_least64_t)

#undef TYPE_SAFE_DETAIL_SELECT

        template <std::size_t Size>
        using flag_set_int = typename select_flag_set_int<Size>::type;
    } // namespace detail

    /// A set of flags where each one can either be `0` or `1`.
    ///
    /// Each enumeration member represents the index of one bit.
    /// \requires The [ts::flag_set_traits]() must be specialized for the given enumeration.
    template <typename Enum>
    class flag_set
    {
        static_assert(std::is_enum<Enum>::value, "not an enum");
        static_assert(flag_set_traits<Enum>::value, "invalid enum for flag_set");

        using traits   = flag_set_traits<Enum>;
        using int_type = detail::flag_set_int<traits::size()>;

        static constexpr int_type get_mask(const Enum& bit) noexcept
        {
            return int_type(int_type(1) << static_cast<std::size_t>(bit));
        }

        static constexpr int_type total_mask() noexcept
        {
            // well-defined due to modulo semantics
            return int_type((int_type(1) << traits::size()) - int_type(1));
        }

    public:
        //=== constructors/assignment ===//
        /// Default constructor.
        /// \effects Creates a set where all bits are set to `0`.
        constexpr flag_set() noexcept : bits_(0)
        {
        }

        /// \effects Creates a set where all bits are set to `0` except the given one.
        constexpr flag_set(const Enum& bit) noexcept : bits_(get_mask(bit))
        {
        }

        /// \effects Same as `*this = flag_set(bit)`.
        flag_set& operator=(const Enum& bit) noexcept
        {
            return *this = flag_set(bit);
        }

        //=== flag operation ===//
        /// \effects Sets the specified bit to `1` (1)/`value` (2/3).
        /// \notes (2) does not participate in overload resolution unless `T` is a boolean-like type.
        /// \group set
        void set(const Enum& bit) noexcept
        {
            bits_ |= get_mask(bit);
        }

        /// \group set
        /// \param 1
        /// \exclude
        template <typename T, typename = detail::enable_boolean<T>>
        void set(const Enum& bit, T value) noexcept
        {
            if (value)
                set(bit);
            else
                reset(bit);
        }

        /// \group set
        void set(const Enum& bit, flag value) noexcept
        {
            set(bit, value == true);
        }

        /// \effects Sets the specified bit to `0`.
        void reset(const Enum& bit) noexcept
        {
            bits_ &= ~get_mask(bit);
        }

        /// \effects Toggles the specified bit.
        void toggle(const Enum& bit) noexcept
        {
            bits_ ^= get_mask(bit);
        }

        /// \effects Sets/resets/toggles all bits.
        /// \group all
        void set_all() noexcept
        {
            bits_ = total_mask();
        }

        /// \group all
        /// \param 1
        /// \exclude
        template <typename T, typename = detail::enable_boolean<T>>
        void set_all(T value) noexcept
        {
            if (value)
                set_all();
            else
                reset_all();
        }

        /// \group all
        void set_all(flag value) noexcept
        {
            set_all(value == true);
        }

        /// \group all
        void reset_all() noexcept
        {
            bits_ = int_type(0);
        }

        /// \group all
        void toggle_all() noexcept
        {
            bits_ ^= total_mask();
        }

        /// \returns Whether or not the specified bit is set.
        constexpr bool is_set(const Enum& bit) const noexcept
        {
            return (bits_ & get_mask(bit)) != int_type(0);
        }

        /// \returns Same as `flag(is_set(bit))`.
        constexpr flag as_flag(const Enum& bit) const noexcept
        {
            return is_set(bit);
        }

        //=== bitwise operations ===//
        /// \returns Whether any bit is set.
        explicit constexpr operator bool() const noexcept
        {
            return bits_ != int_type(0);
        }

        /// \returns An integer where each bit has the value of the corresponding flag.
        /// \requires `T` must be an unsigned integer type with enough bits.
        template <typename T>
        constexpr T to_int() const noexcept
        {
            static_assert(std::is_unsigned<T>::value && sizeof(T) * CHAR_BIT >= traits::size(),
                          "invalid integer type, lossy conversion");
            return bits_;
        }

        /// \returns A set with all bits flipped.
        constexpr flag_set operator~() const noexcept
        {
            return flag_set(~bits_ & total_mask());
        }

        /// \effects Sets all bits in `*this` that are set in `other` (1)/the given `bit` (2).
        /// \notes This is the same as calling the non-argument `set()` function
        /// for each bit set in `other` (1)/for the given `bit` (2).
        /// \group compound_or
        flag_set& operator|=(const flag_set& other) noexcept
        {
            bits_ |= other.bits_;
            return *this;
        }

        /// \group compound_or
        flag_set& operator|=(const Enum& bit) noexcept
        {
            return *this |= flag_set(bit);
        }

        /// \effects Clears all bits that are set in `*this` but not set in `other` (1)/the given bit (2).
        /// \notes This is the same as calling `reset()` for each bit set in `other` (1)/the given bit (2).
        /// \group compound_and
        flag_set& operator&=(const flag_set& other) noexcept
        {
            bits_ &= other.bits_;
            return *this;
        }

        /// \group compound_and
        flag_set& operator&=(const Enum& bit) noexcept
        {
            return *this &= flag_set(bit);
        }

        /// \effects Toggles all bits of `*this` that are set in `other` (1)/the given bit (2).
        /// \notes This is the same as calling `toggle()` for each bit set in `other` (1)/the given bit (2).
        /// \group compound_xor
        flag_set& operator^=(const flag_set& other) noexcept
        {
            bits_ ^= other.bits_;
            return *this;
        }

        /// \group compound_xor
        flag_set& operator^=(const Enum& bit) noexcept
        {
            return *this ^= flag_set(bit);
        }

    private:
        explicit constexpr flag_set(int_type i) noexcept : bits_(i)
        {
        }

        int_type bits_;

        template <typename Enum2>
        friend constexpr bool operator==(const flag_set<Enum2>& a,
                                         const flag_set<Enum2>& b) noexcept;
        template <typename Enum2>
        friend constexpr flag_set<Enum2> operator|(const flag_set<Enum2>& a,
                                                   const flag_set<Enum2>& b) noexcept;
        template <typename Enum2>
        friend constexpr flag_set<Enum2> operator&(const flag_set<Enum2>& a,
                                                   const flag_set<Enum2>& b) noexcept;
        template <typename Enum2>
        friend constexpr flag_set<Enum2> operator^(const flag_set<Enum2>& a,
                                                   const flag_set<Enum2>& b) noexcept;
    };

    /// `flag_set` equality comparison.
    /// \returns Whether both flag sets have the same combination of flags set/not set.
    /// \group flag_set_equal flag_set equality comparison
    template <typename Enum>
    constexpr bool operator==(const flag_set<Enum>& a, const flag_set<Enum>& b) noexcept
    {
        return a.bits_ == b.bits_;
    }

    /// \group flag_set_equal
    template <typename Enum>
    constexpr bool operator==(const flag_set<Enum>& a, const Enum& b) noexcept
    {
        return a == flag_set<Enum>(b);
    }

    /// \group flag_set_equal
    template <typename Enum>
    constexpr bool operator==(const Enum& a, const flag_set<Enum>& b) noexcept
    {
        return flag_set<Enum>(a) == b;
    }

    /// \group flag_set_equal
    template <typename Enum>
    constexpr bool operator!=(const flag_set<Enum>& a, const flag_set<Enum>& b) noexcept
    {
        return !(a == b);
    }

    /// \group flag_set_equal
    template <typename Enum>
    constexpr bool operator!=(const flag_set<Enum>& a, const Enum& b) noexcept
    {
        return !(a == b);
    }

    /// \group flag_set_equal
    template <typename Enum>
    constexpr bool operator!=(const Enum& a, const flag_set<Enum>& b) noexcept
    {
        return !(a == b);
    }

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <typename Enum>                                                                       \
    constexpr flag_set<Enum> operator Op(const flag_set<Enum>& a,                                  \
                                         const flag_set<Enum>& b) noexcept                         \
    {                                                                                              \
        return flag_set<Enum>(a.bits_ Op b.bits_);                                                 \
    }                                                                                              \
    /** \group flag_set_op */                                                                      \
    template <typename Enum>                                                                       \
    constexpr flag_set<Enum> operator Op(const flag_set<Enum>& a, const Enum& b) noexcept          \
    {                                                                                              \
        return a Op flag_set<Enum>(b);                                                             \
    }                                                                                              \
    /** \group flag_set_op */                                                                      \
    template <typename Enum>                                                                       \
    constexpr flag_set<Enum> operator Op(const Enum& a, const flag_set<Enum>& b) noexcept          \
    {                                                                                              \
        return flag_set<Enum>(a) Op b;                                                             \
    }

    /// \returns A new `flag_set` with the corresponding compound bitwise operation applied to it.
    /// \group flag_set_op flag_set bitwise operations
    TYPE_SAFE_DETAIL_MAKE_OP(|)
    /// \group flag_set_op
    TYPE_SAFE_DETAIL_MAKE_OP(&)
    /// \group flag_set_op
    TYPE_SAFE_DETAIL_MAKE_OP (^)

#undef TYPE_SAFE_DETAIL_MAKE_OP
} // namespace type_safe

/// \returns The same as `~ts::flag_set<Enum>(e)`.
/// \notes This function does not participate in overload resolution,
/// unless `Enum` is an `enum` where the [ts::flag_set_traits]() are specialized.
/// \param 1
/// \exclude
template <typename Enum, typename = type_safe::detail::enable_enum_operation<Enum>>
constexpr type_safe::flag_set<Enum> operator~(const Enum& e) noexcept
{
    return ~type_safe::flag_set<Enum>(e);
}

/// \returns The same as `ts::flag_set<Enum>(a) Op b`.
/// \notes These functions do not participate in overload resolution,
/// unless `Enum` is an `enum` where the [ts::flag_set_traits]() are specialized.
/// \group flag_set_enum_op flag_set operation for enums
/// \param 1
/// \exclude
template <typename Enum, typename = type_safe::detail::enable_enum_operation<Enum>>
constexpr type_safe::flag_set<Enum> operator|(const Enum& a, const Enum& b) noexcept
{
    return type_safe::flag_set<Enum>(a) | b;
}

/// \group flag_set_enum_op
/// \param 1
/// \exclude
template <typename Enum, typename = type_safe::detail::enable_enum_operation<Enum>>
constexpr type_safe::flag_set<Enum> operator&(const Enum& a, const Enum& b) noexcept
{
    return type_safe::flag_set<Enum>(a) & b;
}

/// \group flag_set_enum_op
/// \param 1
/// \exclude
template <typename Enum, typename = type_safe::detail::enable_enum_operation<Enum>>
constexpr type_safe::flag_set<Enum> operator^(const Enum& a, const Enum& b) noexcept
{
    return type_safe::flag_set<Enum>(a) ^ b;
}

#endif // TYPE_SAFE_FLAG_SET_HPP_INCLUDED
