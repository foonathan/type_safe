// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
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
        /// Tag type to enable a dynamic bound.
        struct dynamic_bound
        {
        };

        /// \exclude
        namespace detail
        {
            // Base to enable empty base optimization when Bound is not dynamic_bound.
            // Neccessary when T is not a class.
            template <typename T>
            struct wrapper
            {
                using value_type = T;
                T value;
            };

            template <typename Bound>
            struct is_dynamic : std::is_same<Bound, dynamic_bound>
            {
            };

            template <bool Cond, typename T, typename Bound>
            struct select_bound;

            template <typename T, typename Bound>
            struct select_bound<true, T, Bound>
            {
                using type = wrapper<T>;
            };

            template <typename T, typename Bound>
            struct select_bound<false, T, Bound>
            {
                static_assert(
                    std::is_same<T, typename std::decay<decltype(Bound::value)>::type>::value,
                    "static bound has wrong type");
                using type = Bound;
            };

            template <typename T, typename Bound>
            using base = typename select_bound<is_dynamic<Bound>::value, T, Bound>::type;
        } // detail namespace

// clang-format off
/// \exclude
#define TYPE_SAFE_DETAIL_MAKE(Name, Op)                                                            \
    template <typename T, typename Bound = dynamic_bound>                                          \
    class Name : detail::base<T, Bound>                                                            \
    {                                                                                              \
        static constexpr bool is_dynamic = detail::is_dynamic<Bound>::value;                       \
                                                                                                   \
        using base     = detail::base<T, Bound>;                                                   \
        using arg_type = typename std::conditional<is_dynamic, T, Bound>::type;                    \
                                                                                                   \
    public:                                                                                        \
        using value_type = T;                                                                      \
        using bound_type = Bound;                                                                  \
                                                                                                   \
        /** Initializes it with a static bound.
          * \effects Does nothing, a static bound is not stored.
          * It will use `Bound::value` as the bound.
          * \notes This constructor only participates in overload resolution,
          * if a static bound is used, i.e. `Bound` is not [ts::constraints::dynamic_bound]().
          * \param Condition
          * \exclude
          * \param 1
          * \exclude */     \
        template <bool Condition = !is_dynamic,                                                    \
                  typename       = typename std::enable_if<Condition>::type>                       \
        Name(Bound = {})                                                                           \
        {                                                                                          \
        }                                                                                          \
                                                                                                   \
        /** Initializes it with a dynamic bound.
          * \effects Copies (1)/moves (2) the object and uses that as bound.
          * \notes These constructors only participate in overload resoltuion,
          * if a dynamic bound is used, i.e. `Bound` is [ts::constraints::dynamic_bound]().
          * \group dynamic_ctor
          * \param Condition
          * \exclude
          * \param 1
          * \exclude */     \
        template <bool Condition = is_dynamic,                                                     \
                  typename       = typename std::enable_if<Condition>::type>                       \
        explicit Name(const T& bound) : base{bound}                                                \
        {                                                                                          \
        }                                                                                          \
                                                                                                   \
        /** \group dynamic_ctor
          * \param Condition
          * \exclude
          * \param 1
          * \exclude */     \
        template <bool Condition = is_dynamic,                                                     \
                  typename       = typename std::enable_if<Condition>::type>                       \
        explicit Name(T&& bound) noexcept(std::is_nothrow_move_constructible<T>::value)            \
        : base{std::move(bound)}                                                                   \
        {                                                                                          \
        }                                                                                          \
                                                                                                   \
        /** Does the actual bounds check.*/                                                        \
        template <typename U>                                                                      \
        bool operator()(const U& u) const                                                          \
        {                                                                                          \
            return u Op get_bound();                                                               \
        }                                                                                          \
                                                                                                   \
        /** \returns The bound.*/                                                                  \
        const T& get_bound() const noexcept                                                        \
        {                                                                                          \
            return base::value;                                                                    \
        }                                                                                          \
    };
        // clang-format on

        /// A `Constraint` for the [ts::constrained_type]().
        ///
        /// A value is valid if it is less than some given value.
        TYPE_SAFE_DETAIL_MAKE(less, <)

        /// A `Constraint` for the [ts::constrained_type]().
        ///
        /// A value is valid if it is less than or equal to some given value.
        TYPE_SAFE_DETAIL_MAKE(less_equal, <=)

        /// A `Constraint` for the [ts::constrained_type]().
        ///
        /// A value is valid if it is greater than some given value.
        TYPE_SAFE_DETAIL_MAKE(greater, >)

        /// A `Constraint` for the [ts::constrained_type]().
        ///
        /// A value is valid if it is greater than or equal to some given value.
        TYPE_SAFE_DETAIL_MAKE(greater_equal, >=)

#undef TYPE_SAFE_DETAIL_MAKE

        /// \exclude
        namespace detail
        {
            // checks that that the value is less than the upper bound
            template <bool Inclusive, typename T, typename Bound>
            using upper_bound_t =
                typename std::conditional<Inclusive, less_equal<T, Bound>, less<T, Bound>>::type;

            // checks that the value is greater than the lower bound
            template <bool Inclusive, typename T, typename Bound>
            using lower_bound_t = typename std::conditional<Inclusive, greater_equal<T, Bound>,
                                                            greater<T, Bound>>::type;
        } // namespace detail

        /// Tag objects to specify bounds for [ts::constraints::bounded]().
        /// \group open_closed Open/Closed Tags
        constexpr bool open = false;
        /// \group open_closed
        constexpr bool closed = true;

        /// A `Constraint` for the [ts::constrained_type]().
        ///
        /// A value is valid if it is between two given bounds,
        /// `LowerInclusive`/`UpperInclusive` control whether the lower/upper bound itself is valid too.
        template <typename T, bool LowerInclusive, bool UpperInclusive,
                  typename LowerBound = dynamic_bound, typename UpperBound = dynamic_bound>
        class bounded : detail::lower_bound_t<LowerInclusive, T, LowerBound>,
                        detail::upper_bound_t<UpperInclusive, T, UpperBound>
        {
            static constexpr bool lower_is_dynamic = detail::is_dynamic<LowerBound>::value;
            static constexpr bool upper_is_dynamic = detail::is_dynamic<UpperBound>::value;

            using lower_type = detail::lower_bound_t<LowerInclusive, T, LowerBound>;
            using upper_type = detail::upper_bound_t<UpperInclusive, T, UpperBound>;

            const lower_type& lower() const noexcept
            {
                return *this;
            }
            const upper_type& upper() const noexcept
            {
                return *this;
            }

            template <typename U>
            using decay_same = std::is_same<typename std::decay<U>::type, T>;

        public:
            using value_type  = T;
            using lower_bound = LowerBound;
            using upper_bound = UpperBound;

            static constexpr auto lower_inclusive = LowerInclusive;
            static constexpr auto upper_inclusive = UpperInclusive;

            /// Initializes it with static bounds.
            /// \effects Does nothing, a static bound is not stored.
            /// It will use `LowerBound::value` as lower bound and `UpperBound::value` as upper bound.
            /// \notes This constructor does not participate in overload resolution,
            /// unless both bounds are static,
            /// i.e. not [ts::constraints::dynamic_bound]().
            /// \param Condition
            /// \exclude
            /// \param 1
            /// \exclude
            template <bool Condition = !lower_is_dynamic && !upper_is_dynamic,
                      typename       = typename std::enable_if<Condition>::type>
            bounded()
            {
            }

            /// Initializes it with (mixed) dynamic bounds.
            /// \effects Perfectly forwards the arguments to the bounds.
            /// If a bound is static, the static member `value` will be used as bound,
            /// if it is dynamic, a copy created by perfectly forwarding will be stored and used as bound.
            /// \notes This constructor does not participate in overload resolution,
            /// unless the arguments are convertible to the bounds.
            /// \param 2
            /// \exclude
            /// \param 3
            /// \exclude
            template <typename U1, typename U2>
            explicit bounded(U1&& lower, U2&& upper,
                             decltype(lower_type(std::forward<U1>(lower)), 0) = 0,
                             decltype(upper_type(std::forward<U2>(upper)), 0) = 0)
            : lower_type(std::forward<U1>(lower)), upper_type(std::forward<U2>(upper))
            {
            }

            /// Does the bounds check.
            template <typename U>
            bool operator()(const U& u) const
            {
                return lower()(u) && upper()(u);
            }

            /// \returns The value of the lower bound.
            const T& get_lower_bound() const noexcept
            {
                return lower().get_bound();
            }

            /// \returns The value of the upper bound.
            const T& get_upper_bound() const noexcept
            {
                return upper().get_bound();
            }
        };

        /// A `Constraint` for the [ts::constrained_type]().
        ///
        /// A value is valid if it is between two given bounds but not the bounds themselves.
        template <typename T, typename LowerBound = dynamic_bound,
                  typename UpperBound = dynamic_bound>
        using open_interval           = bounded<T, open, open, LowerBound, UpperBound>;

        /// A `Constraint` for the [ts::constrained_type]().
        ///
        /// A value is valid if it is between two given bounds or the bounds themselves.
        template <typename T, typename LowerBound = dynamic_bound,
                  typename UpperBound = dynamic_bound>
        using closed_interval         = bounded<T, closed, closed, LowerBound, UpperBound>;
    } // namespace constraints

    /// \exclude
    namespace detail
    {
        template <typename T>
        struct valid_bound : std::integral_constant<bool, true || T::value>
        {
        };

        template <class Verifier, bool LowerInclusive, bool UpperInclusive, typename T, typename U1,
                  typename U2>
        struct bounded_type_impl
        {
            static_assert(valid_bound<U1>::value && valid_bound<U2>::value,
                          "make_bounded() called with mismatched types");

            using value_type     = T;
            using lower_constant = U1;
            using upper_constant = U2;

            using type =
                constrained_type<value_type,
                                 constraints::bounded<value_type, LowerInclusive, UpperInclusive,
                                                      lower_constant, upper_constant>,
                                 Verifier>;
        };

        template <class Verifier, bool LowerInclusive, bool UpperInclusive, typename T>
        struct bounded_type_impl<Verifier, LowerInclusive, UpperInclusive, T, T, T>
        {
            using value_type     = T;
            using lower_constant = constraints::dynamic_bound;
            using upper_constant = constraints::dynamic_bound;

            using type =
                constrained_type<value_type,
                                 constraints::bounded<value_type, LowerInclusive, UpperInclusive,
                                                      lower_constant, upper_constant>,
                                 Verifier>;
        };

        template <class Verifier, bool LowerInclusive, bool UpperInclusive, typename T,
                  typename UpperBound>
        struct bounded_type_impl<Verifier, LowerInclusive, UpperInclusive, T, T, UpperBound>
        {
            static_assert(valid_bound<UpperBound>::value,
                          "make_bounded() called with mismatched types");

            using value_type     = T;
            using lower_constant = constraints::dynamic_bound;
            using upper_constant = UpperBound;

            using type =
                constrained_type<value_type,
                                 constraints::bounded<value_type, LowerInclusive, UpperInclusive,
                                                      lower_constant, upper_constant>,
                                 Verifier>;
        };

        template <class Verifier, bool LowerInclusive, bool UpperInclusive, typename T,
                  typename LowerBound>
        struct bounded_type_impl<Verifier, LowerInclusive, UpperInclusive, T, LowerBound, T>
        {
            static_assert(valid_bound<LowerBound>::value,
                          "make_bounded() called with mismatched types");

            using value_type     = T;
            using lower_constant = LowerBound;
            using upper_constant = constraints::dynamic_bound;

            using type =
                constrained_type<value_type,
                                 constraints::bounded<value_type, LowerInclusive, UpperInclusive,
                                                      lower_constant, upper_constant>,
                                 Verifier>;
        };

        template <class Verifier, bool LowerInclusive, bool UpperInclusive, typename T, typename U1,
                  typename U2>
        using make_bounded_type =
            typename bounded_type_impl<Verifier, LowerInclusive, UpperInclusive,
                                       typename std::decay<T>::type, typename std::decay<U1>::type,
                                       typename std::decay<U2>::type>::type;
    } // namespace detail

    /// An alias for [ts::constrained_type]() that uses [ts::constraints::bounded]() as its `Constraint`.
    /// \notes This is some type where the values must be in a certain interval.
    template <typename T, bool LowerInclusive, bool UpperInclusive,
              typename LowerBound = constraints::dynamic_bound,
              typename UpperBound = constraints::dynamic_bound>
    using bounded_type = constrained_type<T, constraints::bounded<T, LowerInclusive, UpperInclusive,
                                                                  LowerBound, UpperBound>,
                                          assertion_verifier>;

    /// Creates a [ts::bounded_type]() to a specified [ts::constraints::closed_interval]().
    /// \returns A [ts::bounded_type]() with the given `value` and lower and upper bounds,
    /// where the bounds are valid values as well.
    /// \notes If this function is passed in dynamic values of the same type as `value`,
    /// it will create a dynamic bound.
    /// Otherwise it must be passed static bounds.
    template <typename T, typename U1, typename U2>
    auto make_bounded(T&& value, U1&& lower, U2&& upper)
        -> detail::make_bounded_type<assertion_verifier, true, true, T, U1, U2>
    {
        using result_type = detail::make_bounded_type<assertion_verifier, true, true, T, U1, U2>;
        return result_type(std::forward<T>(value),
                           typename result_type::constraint_predicate(std::forward<U1>(lower),
                                                                      std::forward<U2>(upper)));
    }

    /// Creates a [ts::bounded_type]() to a specified [ts::constraints::open_interval]().
    /// \returns A [ts::bounded_type]() with the given `value` and lower and upper bounds,
    /// where the bounds are not valid values.
    /// \notes If this function is passed in dynamic values of the same type as `value`,
    /// it will create a dynamic bound.
    /// Otherwise it must be passed static bounds.
    template <typename T, typename U1, typename U2>
    auto make_bounded_exclusive(T&& value, U1&& lower, U2&& upper)
        -> detail::make_bounded_type<assertion_verifier, false, false, T, U1, U2>
    {
        using result_type = detail::make_bounded_type<assertion_verifier, false, false, T, U1, U2>;
        return result_type(std::forward<T>(value),
                           typename result_type::constraint_predicate(std::forward<U1>(lower),
                                                                      std::forward<U2>(upper)));
    }

    /// Changes `val` so that it is in the given [ts::constraints::closed_interval]().
    /// \effects If it is not in the interval, assigns the bound that is closer to the value.
    template <typename T, typename LowerBound, typename UpperBound, typename U>
    void clamp(const constraints::closed_interval<T, LowerBound, UpperBound>& interval, U& val)
    {
        if (val < interval.get_lower_bound())
            val = static_cast<U>(interval.get_lower_bound());
        else if (val > interval.get_upper_bound())
            val = static_cast<U>(interval.get_upper_bound());
    }

    /// A `Verifier` for [ts::constrained_type]() that clamps the value to make it valid.
    ///
    /// It must be used together with [ts::constraints::less_equal](), [ts::constraints::greater_equal]() or [ts::constraints::closed_interval]().
    struct clamping_verifier
    {
        /// \effects If `val` is greater than the bound of `p`,
        /// assigns the bound to `val`.
        /// Otherwise does nothing.
        template <typename Value, typename T, typename Bound>
        static void verify(Value& val, const constraints::less_equal<T, Bound>& p)
        {
            if (!p(val))
                val = static_cast<Value>(p.get_bound());
        }

        /// \effects If `val` is less than the bound of `p`,
        /// assigns the bound to `val`.
        /// Otherwise does nothing.
        template <typename Value, typename T, typename Bound>
        static void verify(Value& val, const constraints::greater_equal<T, Bound>& p)
        {
            if (!p(val))
                val = static_cast<Value>(p.get_bound());
        }

        /// \effects Same as `clamp(interval, val)`.
        template <typename Value, typename T, typename LowerBound, typename UpperBound>
        static void verify(Value& val,
                           const constraints::closed_interval<T, LowerBound, UpperBound>& interval)
        {
            clamp(interval, val);
        }
    };

    /// An alias for [ts::constrained_type]() that uses [ts::constraints::closed_interval]() as its `Constraint`
    /// and [ts::clamping_verifier]() as its `Verifier`.
    /// \notes This is some type where the values are always clamped so that they are in a certain interval.
    template <typename T, typename LowerBound = constraints::dynamic_bound,
              typename UpperBound = constraints::dynamic_bound>
    using clamped_type =
        constrained_type<T, constraints::closed_interval<T, LowerBound, UpperBound>,
                         clamping_verifier>;

    /// Creates a [ts::clamped_type]() from the specified [ts::constraints::closed_interval]().
    /// \returns A [ts::clamped_type]() with the given `value` and lower and upper bounds,
    /// where the bounds are valid values.
    /// \notes If this function is passed in dynamic values of the same type as `value`,
    /// it will create a dynamic bound.
    /// Otherwise it must be passed static bounds.
    template <typename T, typename U1, typename U2>
    auto make_clamped(T&& value, U1&& lower, U2&& upper)
        -> detail::make_bounded_type<clamping_verifier, true, true, T, U1, U2>
    {
        using result_type = detail::make_bounded_type<clamping_verifier, true, true, T, U1, U2>;
        return result_type(std::forward<T>(value),
                           typename result_type::constraint_predicate(std::forward<U1>(lower),
                                                                      std::forward<U2>(upper)));
    }
} // namespace type_safe

#endif // TYPE_SAFE_BOUNDED_TYPE_HPP_INCLUDED
