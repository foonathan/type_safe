// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_INDEX_HPP_INCLUDED
#define TYPE_SAFE_INDEX_HPP_INCLUDED

#include <cstddef>

#include <type_safe/config.hpp>
#include <type_safe/integer.hpp>
#include <type_safe/strong_typedef.hpp>

namespace type_safe
{
    namespace detail
    {
#if TYPE_SAFE_ENABLE_WRAPPER
        using index_t    = integer<std::size_t, undefined_behavior_arithmetic>;
        using distance_t = integer<std::ptrdiff_t, undefined_behavior_arithmetic>;
#else
        using index_t    = std::size_t;
        using distance_t = std::ptrdiff_t;
#endif
    } // namespace detail

    /// A type modelling the distance between two [type_safe::index_t]() objects.
    ///
    /// It is a [type_safe::strong_typedef<Tag, T>]() for a `signed` integer type.
    /// It is comparable and you can add and subtract two differences.
    struct distance_t : strong_typedef<distance_t, detail::distance_t>,
                        strong_typedef_op::equality_comparison<distance_t>,
                        strong_typedef_op::relational_comparison<distance_t>,
                        strong_typedef_op::unary_plus<distance_t>,
                        strong_typedef_op::unary_minus<distance_t>,
                        strong_typedef_op::addition<distance_t>,
                        strong_typedef_op::subtraction<distance_t>
    {
        using strong_typedef::strong_typedef;

        /// \effects Initializes it to `0`.
        constexpr distance_t() noexcept : strong_typedef(0)
        {
        }
    };

    /// A type modelling an index into an array.
    ///
    /// It is a [type_safe::strong_typedef<Tag, T>]() for an `unsigned` integer type.
    /// It is comparable and you can increment and decrement it,
    /// as well as adding/subtracing a [type_safe::distance_t]().
    /// \notes It has a similar interface to a `RandomAccessIterator`,
    /// but without the dereference functions.
    struct index_t : strong_typedef<index_t, detail::index_t>,
                     strong_typedef_op::equality_comparison<index_t>,
                     strong_typedef_op::relational_comparison<index_t>,
                     strong_typedef_op::increment<index_t>,
                     strong_typedef_op::decrement<index_t>,
                     strong_typedef_op::unary_plus<index_t>
    {
        using strong_typedef::strong_typedef;

        /// \effects Initializes it to `0`.
        constexpr index_t() noexcept : strong_typedef(0u)
        {
        }

        /// \effects Advances the index by the distance specified in `rhs`.
        /// If `rhs` is a negative distance, it advances backwards.
        /// \requires The new index must be greater or equal to `0`.
        index_t& operator+=(const distance_t& rhs) noexcept
        {
            get(*this) = make_unsigned(make_signed(get(*this)) + get(rhs));
            return *this;
        }

        /// \effects Advances the index backwards by the distance specified in `rhs`.
        /// If `rhs` is a negative distance, it advances forwards.
        /// \requires The new index must be greater or equal to `0`.
        index_t& operator-=(const distance_t& rhs) noexcept
        {
            get(*this) = make_unsigned(make_signed(get(*this)) - get(rhs));
            return *this;
        }
    };

    /// \returns Same as `lhs += rhs`, but does not modify `lhs`,
    /// returns a copy instead.
    constexpr index_t operator+(const index_t& lhs, const distance_t& rhs) noexcept
    {
        return index_t(make_unsigned(make_signed(get(lhs)) + get(rhs)));
    }

    /// \returns Same as `rhs + lhs`.
    constexpr index_t operator+(const distance_t& lhs, const index_t& rhs) noexcept
    {
        return rhs + lhs;
    }

    /// \returns Same as `lhs -= rhs`, but does not modify `lhs`,
    /// returns a copy instead.
    constexpr index_t operator-(const index_t& lhs, const distance_t& rhs) noexcept
    {
        return index_t(make_unsigned(make_signed(get(lhs)) - get(rhs)));
    }

    /// \returns Returns the distance between two indices.
    /// This is the number of steps you need to increment `lhs` to reach `rhs`,
    /// it is negative if `lhs > rhs`.
    constexpr distance_t operator-(const index_t& lhs, const index_t& rhs) noexcept
    {
        return distance_t(make_signed(get(lhs)) - make_signed(get(rhs)));
    }

    namespace detail
    {
        struct no_size
        {
        };

        template <typename Indexable>
        bool index_valid(no_size, const Indexable&, const std::size_t&)
        {
            return true;
        }

        struct non_member_size : no_size
        {
        };

        template <typename Indexable>
        auto index_valid(non_member_size, const Indexable& obj, const std::size_t& index)
            -> decltype(index < size(obj))
        {
            return index < size(obj);
        }

        struct member_size : non_member_size
        {
        };

        template <typename Indexable>
        auto index_valid(member_size, const Indexable& obj, const std::size_t& index)
            -> decltype(index < obj.size())
        {
            return index < obj.size();
        }

        template <typename T, std::size_t Size>
        bool index_valid(member_size, const T (&)[Size], const std::size_t& index)
        {
            return index < Size;
        }
    } // namespace detail

    /// \returns The `i`th element of `obj` by invoking its `operator[]` with the index converted to `std::size_t`.
    /// \requires `index` must be a valid index for `obj`,
    /// i.e. less than the size of `obj`.
    template <typename Indexable>
    auto at(Indexable&& obj, const index_t& index)
        -> decltype(std::forward<Indexable>(obj)[static_cast<std::size_t>(get(index))])
    {
        DEBUG_ASSERT(detail::index_valid(detail::member_size{}, obj,
                                         static_cast<std::size_t>(get(index))),
                     detail::assert_handler{});
        return std::forward<Indexable>(obj)[static_cast<std::size_t>(get(index))];
    }

    /// \effects Increments the index by the specified distance.
    /// If the distance is negative, decrements the index instead.
    /// \notes This is the same as `index += dist` and the equivalent of [std::advance()]().
    void advance(index_t& index, const distance_t& dist)
    {
        index += dist;
    }

    /// \returns The distance between `a` and `b`,
    /// i.e. how often you'd have to increment `a` to reach `b`.
    /// \notes This is the same as `b - a` and the equivalent of [std::distance()]().
    constexpr distance_t distance(const index_t& a, const index_t& b)
    {
        return b - a;
    }

    /// \returns The index that is `dist` greater than `index`.
    /// \notes This is the same as `index + dist` and the equivalent of [std::next()]().
    constexpr index_t next(const index_t& index, const distance_t& dist = distance_t(1))
    {
        return index + dist;
    }

    /// \returns The index that is `dist` smaller than `index`.
    /// \notes This is the same as `index - dist` and the equivalent of [std::prev()]().
    constexpr index_t prev(const index_t& index, const distance_t& dist = distance_t(1))
    {
        return index - dist;
    }
} // namespace type_safe

#endif // TYPE_SAFE_INDEX_HPP_INCLUDED
