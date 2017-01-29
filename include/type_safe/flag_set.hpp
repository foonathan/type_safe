// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_FLAG_SET_HPP_INCLUDED
#define TYPE_SAFE_FLAG_SET_HPP_INCLUDED

#include <bitset>
#include <type_traits>

#include <type_safe/flag.hpp>
#include <type_safe/types.hpp>

namespace type_safe
{
    /// Traits for the enum used in a [ts::flag_set]().
    ///
    /// Specialize it for your own enumeration type.
    /// It must inherit from [std::true_type]() in order to create the combination operations.
    /// It also must provide a `static constexpr` function `size()` that returns the number of enumeration values
    /// used for indexing.
    /// \requires The enumeration must be contiguous starting at `0`, simply don't set an explicit value to the enumeration members.
    template <typename Enum>
    struct flag_set_traits : std::false_type
    {
    };

    /// \exclude
    namespace detail
    {
        template <typename Enum>
        using enable_enum_operation =
            typename std::enable_if<std::is_enum<Enum>::value
                                    && flag_set_traits<Enum>::value>::type;
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

        using traits = flag_set_traits<Enum>;
        using bitset = std::bitset<traits::size()>;

    public:
        //=== constructors/assignment ===//
        /// Default constructor.
        /// \effects Creates a set where all bits are set to `0`.
        flag_set() noexcept = default;

        /// \effects Creates a set where all bits are set to `0` except the given one.
        flag_set(const Enum& bit) noexcept
        {
            bits_.set(static_cast<std::size_t>(bit));
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
            bits_.set(static_cast<std::size_t>(bit));
        }

        /// \group set
        /// \param 1
        /// \exclude
        template <typename T, typename = detail::enable_boolean<T>>
        void set(const Enum& bit, T value) noexcept
        {
            bits_.set(static_cast<std::size_t>(bit), static_cast<bool>(value));
        }

        /// \group set
        void set(const Enum& bit, flag value) noexcept
        {
            bits_.set(static_cast<std::size_t>(bit), value == true);
        }

        /// \effects Sets the specified bit to `0`.
        void reset(const Enum& bit) noexcept
        {
            bits_.reset(static_cast<std::size_t>(bit));
        }

        /// \effects Toggles the specified bit.
        void toggle(const Enum& bit) noexcept
        {
            bits_.flip(static_cast<std::size_t>(bit));
        }

        /// \effects Sets/resets/toggles all bits.
        /// \group all
        void set_all() noexcept
        {
            bits_.set();
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
            bits_.reset();
        }

        /// \group all
        void toggle_all() noexcept
        {
            bits_.flip();
        }

        /// \returns Whether or not the specified bit is set.
        bool is_set(const Enum& bit) const noexcept
        {
            return bits_[static_cast<std::size_t>(bit)];
        }

        /// \returns Same as `flag(is_set(bit))`.
        flag as_flag(const Enum& bit) const noexcept
        {
            return is_set(bit);
        }

        //=== bitwise operations ===//
        /// \returns Whether any bit is set.
        explicit operator bool() const noexcept
        {
            return bits_.any();
        }

        /// \returns A set with all bits flipped.
        flag_set operator~() const noexcept
        {
            flag_set result;
            result.bits_ = ~bits_;
            return result;
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
        bitset bits_;

        template <typename Enum2>
        friend bool operator==(const flag_set<Enum2>& a, const flag_set<Enum2>& b) noexcept;
    };

    /// `flag_set` equality comparison.
    /// \returns Whether both flag sets have the same combination of flags set/not set.
    /// \group flag_set_equal flag_set equality comparison
    template <typename Enum>
    bool operator==(const flag_set<Enum>& a, const flag_set<Enum>& b) noexcept
    {
        return a.bits_ == b.bits_;
    }

    /// \group flag_set_equal
    template <typename Enum>
    bool operator==(const flag_set<Enum>& a, const Enum& b) noexcept
    {
        return a == flag_set<Enum>(b);
    }

    /// \group flag_set_equal
    template <typename Enum>
    bool operator==(const Enum& a, const flag_set<Enum>& b) noexcept
    {
        return flag_set<Enum>(a) == b;
    }

    /// \group flag_set_equal
    template <typename Enum>
    bool operator!=(const flag_set<Enum>& a, const flag_set<Enum>& b) noexcept
    {
        return !(a == b);
    }

    /// \group flag_set_equal
    template <typename Enum>
    bool operator!=(const flag_set<Enum>& a, const Enum& b) noexcept
    {
        return !(a == b);
    }

    /// \group flag_set_equal
    template <typename Enum>
    bool operator!=(const Enum& a, const flag_set<Enum>& b) noexcept
    {
        return !(a == b);
    }

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op)                                                               \
    template <typename Enum>                                                                       \
    flag_set<Enum> operator Op(const flag_set<Enum>& a, const flag_set<Enum>& b) noexcept          \
    {                                                                                              \
        flag_set<Enum> result = a;                                                                 \
        result         Op##   = b;                                                                 \
        return result;                                                                             \
    }                                                                                              \
    /** \group flag_set_op */                                                                      \
    template <typename Enum>                                                                       \
    flag_set<Enum> operator Op(const flag_set<Enum>& a, const Enum& b) noexcept                    \
    {                                                                                              \
        flag_set<Enum> result = a;                                                                 \
        result         Op##   = b;                                                                 \
        return result;                                                                             \
    }                                                                                              \
    /** \group flag_set_op */                                                                      \
    template <typename Enum>                                                                       \
    flag_set<Enum> operator Op(const Enum& a, const flag_set<Enum>& b) noexcept                    \
    {                                                                                              \
        flag_set<Enum> result = a;                                                                 \
        result         Op##   = b;                                                                 \
        return result;                                                                             \
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
type_safe::flag_set<Enum> operator~(const Enum& e) noexcept
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
type_safe::flag_set<Enum> operator|(const Enum& a, const Enum& b) noexcept
{
    return type_safe::flag_set<Enum>(a) | b;
}

/// \group flag_set_enum_op
/// \param 1
/// \exclude
template <typename Enum, typename = type_safe::detail::enable_enum_operation<Enum>>
type_safe::flag_set<Enum> operator&(const Enum& a, const Enum& b) noexcept
{
    return type_safe::flag_set<Enum>(a) & b;
}

/// \group flag_set_enum_op
/// \param 1
/// \exclude
template <typename Enum, typename = type_safe::detail::enable_enum_operation<Enum>>
type_safe::flag_set<Enum> operator^(const Enum& a, const Enum& b) noexcept
{
    return type_safe::flag_set<Enum>(a) ^ b;
}

#endif // TYPE_SAFE_FLAG_SET_HPP_INCLUDED
