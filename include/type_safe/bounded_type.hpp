// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_BOUNDED_TYPE_HPP_INCLUDED
#define TYPE_SAFE_BOUNDED_TYPE_HPP_INCLUDED

#include <type_traits>

#include <type_safe/constrained_type.hpp>

namespace type_safe
{
    namespace constraints
    {
#define TYPE_SAFE_DETAIL_MAKE(Name, Op)                                                            \
    template <typename T>                                                                          \
    class Name                                                                                     \
    {                                                                                              \
    public:                                                                                        \
        explicit Name(const T& bound) : bound_(bound)                                              \
        {                                                                                          \
        }                                                                                          \
                                                                                                   \
        explicit Name(T&& bound) noexcept(std::is_nothrow_move_constructible<T>::value)            \
        : bound_(std::move(bound))                                                                 \
        {                                                                                          \
        }                                                                                          \
                                                                                                   \
        template <typename U>                                                                      \
        bool operator()(const U& u) const                                                          \
        {                                                                                          \
            return u Op bound_;                                                                    \
        }                                                                                          \
                                                                                                   \
        const T& get_bound() const noexcept                                                        \
        {                                                                                          \
            return bound_;                                                                         \
        }                                                                                          \
                                                                                                   \
    private:                                                                                       \
        T bound_;                                                                                  \
    };

        /// A `Constraint` for the [type_safe::constrained_type<T, Constraint, Verifier>]().
        /// A value is valid if it is less than some given value.
        TYPE_SAFE_DETAIL_MAKE(less, <)

        /// A `Constraint` for the [type_safe::constrained_type<T, Constraint, Verifier>]().
        /// A value is valid if it is less than or equal to some given value.
        TYPE_SAFE_DETAIL_MAKE(less_equal, <=)

        /// A `Constraint` for the [type_safe::constrained_type<T, Constraint, Verifier>]().
        /// A value is valid if it is greater than some given value.
        TYPE_SAFE_DETAIL_MAKE(greater, >)

        /// A `Constraint` for the [type_safe::constrained_type<T, Constraint, Verifier>]().
        /// A value is valid if it is greater than or equal to some given value.
        TYPE_SAFE_DETAIL_MAKE(greater_equal, >=)

#undef TYPE_SAFE_DETAIL_MAKE

        namespace detail
        {
            // checks that that the value is less than the upper bound
            template <bool Inclusive, typename T>
            using upper_bound_t =
                typename std::conditional<Inclusive, less_equal<T>, less<T>>::type;

            // checks that the value is greater than the lower bound
            template <bool Inclusive, typename T>
            using lower_bound_t =
                typename std::conditional<Inclusive, greater_equal<T>, greater<T>>::type;
        } // namespace detail

        constexpr bool open   = false;
        constexpr bool closed = true;

        /// A `Constraint` for the [type_safe::constrained_type<T, Constraint, Verifier>]().
        /// A value is valid if it is between two given bounds,
        /// `LowerInclusive`/`UpperInclusive` control whether the lower/upper bound itself is valid too.
        template <typename T, bool LowerInclusive, bool UpperInclusive>
        class bounded
        {
            template <typename U>
            using decay_same = std::is_same<typename std::decay<U>::type, T>;

        public:
            template <typename U1, typename U2,
                      typename = typename std::enable_if<decay_same<U1>::value>::type,
                      typename = typename std::enable_if<decay_same<U2>::value>::type>
            bounded(U1&& lower, U2&& upper)
            : lower_(std::forward<U1>(lower)), upper_(std::forward<U2>(upper))
            {
            }

            template <typename U>
            bool operator()(const U& u) const
            {
                return lower_(u) && upper_(u);
            }

            const T& get_lower_bound() const noexcept
            {
                return lower_.get_bound();
            }

            const T& get_upper_bound() const noexcept
            {
                return upper_.get_bound();
            }

        private:
            detail::lower_bound_t<LowerInclusive, T> lower_;
            detail::upper_bound_t<UpperInclusive, T> upper_;
        };

        /// A `Constraint` for the [type_safe::constrained_type<T, Constraint, Verifier>]().
        /// A value is valid if it is between two given bounds but not the bounds themselves.
        template <typename T>
        using open_interval = bounded<T, open, open>;

        /// A `Constraint` for the [type_safe::constrained_type<T, Constraint, Verifier>]().
        /// A value is valid if it is between two given bounds or the bounds themselves.
        template <typename T>
        using closed_interval = bounded<T, closed, closed>;
    } // namespace constraints

    /// \exclude
    namespace detail
    {
        template <typename T, typename U1, typename U2>
        struct bounded_value_type_impl
        {
            static_assert(!std::is_same<T, U1>::value && !std::is_same<U1, U2>::value,
                          "make_bounded() called with mismatching types");
        };

        template <typename T>
        struct bounded_value_type_impl<T, T, T>
        {
            using type = T;
        };

        template <typename T, typename U1, typename U2>
        using bounded_value_type =
            typename bounded_value_type_impl<typename std::decay<T>::type,
                                             typename std::decay<U1>::type,
                                             typename std::decay<U2>::type>::type;
    } // namespace detail

    /// An alias for [type_safe::constrained_type<T, Constraint, Verifier>]() that uses [type_safe::constraints::bounded<T, LowerInclusive, UpperInclusive>]() as its `Constraint`.
    /// \notes This is some type where the values must be in a certain interval.
    template <typename T, bool LowerInclusive, bool UpperInclusive>
    using bounded_type =
        constrained_type<T, constraints::bounded<T, LowerInclusive, UpperInclusive>>;

    /// \returns A [type_safe::bounded_type<T, LowerInclusive, UpperInclusive>]() with the given `value` and lower and upper bounds,
    /// where those bounds are valid values as well.
    template <typename T, typename U1, typename U2>
    auto make_bounded(T&& value, U1&& lower, U2&& upper)
        -> bounded_type<detail::bounded_value_type<T, U1, U2>, true, true>
    {
        using value_type = detail::bounded_value_type<T, U1, U2>;
        return bounded_type<value_type, true,
                            true>(std::forward<T>(value),
                                  constraints::closed_interval<value_type>(std::forward<U1>(lower),
                                                                           std::forward<U2>(
                                                                               upper)));
    }

    /// \returns A [type_safe::bounded_type<T, LowerInclusive, UpperInclusive>]() with the given `value` and lower and upper bounds,
    /// where those bounds are not valid values.
    template <typename T, typename U1, typename U2>
    auto make_bounded_exclusive(T&& value, U1&& lower, U2&& upper)
        -> bounded_type<detail::bounded_value_type<T, U1, U2>, false, false>
    {
        using value_type = detail::bounded_value_type<T, U1, U2>;
        return bounded_type<value_type, false,
                            false>(std::forward<T>(value),
                                   constraints::open_interval<value_type>(std::forward<U1>(lower),
                                                                          std::forward<U2>(upper)));
    }

    /// \effects Changes `val` so that it is in the interval.
    /// If it is not in the interval, assigns the bound that is closer to the value.
    template <typename T, typename U>
    void clamp(const constraints::closed_interval<T>& interval, U& val)
    {
        if (val < interval.get_lower_bound())
            val = static_cast<U>(interval.get_lower_bound());
        else if (val > interval.get_upper_bound())
            val = static_cast<U>(interval.get_upper_bound());
    }

    /// A `Verifier` for [type_safe::constrained_type<T, Constraint, Verifier]() that clamps the value to make it valid.
    /// It must be used together with [type_safe::constraints::less_equal<T>](), [type_safe::constraints::greater_equal<T>]() or [type_safe::constraints::closed_interval<T>]().
    struct clamping_verifier
    {
        /// \effects If `val` is greater than the bound of `p`,
        /// assigns the bound to `val`.
        /// Otherwise does nothing.
        template <typename Value, typename T>
        static void verify(Value& val, const constraints::less_equal<T>& p)
        {
            if (!p(val))
                val = static_cast<Value>(p.get_bound());
        }

        /// \effects If `val` is less than the bound of `p`,
        /// assigns the bound to `val`.
        /// Otherwise does nothing.
        template <typename Value, typename T>
        static void verify(Value& val, const constraints::greater_equal<T>& p)
        {
            if (!p(val))
                val = static_cast<Value>(p.get_bound());
        }

        /// \effects Same as `clamp(interval, val)`.
        template <typename Value, typename T>
        static void verify(Value& val, const constraints::closed_interval<T>& interval)
        {
            clamp(interval, val);
        }
    };

    /// An alias for [type_safe::constrained_type<T, Constraint, Verifier>]() that uses [type_safe::constraints::closed_interval<T>]() as its `Constraint`
    /// and [type_safe::clamping_verifier]() as its `Verifier`.
    /// \notes This is some type where the values are always clamped so that they are in a certain interval.
    template <typename T>
    using clamped_type = constrained_type<T, constraints::closed_interval<T>, clamping_verifier>;

    /// \returns A [type_safe::clamped_type<T>]() with the given `value` and lower and upper bounds.
    template <typename T, typename U1, typename U2>
    auto make_clamped(T&& value, U1&& lower, U2&& upper)
        -> clamped_type<detail::bounded_value_type<T, U1, U2>>
    {
        using value_type = detail::bounded_value_type<T, U1, U2>;
        return clamped_type<value_type>(std::forward<T>(value),
                                        constraints::closed_interval<value_type>(std::forward<U1>(
                                                                                     lower),
                                                                                 std::forward<U2>(
                                                                                     upper)));
    }
} // namespace type_safe

#endif // TYPE_SAFE_BOUNDED_TYPE_HPP_INCLUDED
