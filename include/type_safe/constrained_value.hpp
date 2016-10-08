// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_CONSTRAINED_VALUE_HPP_INCLUDED
#define TYPE_SAFE_CONSTRAINED_VALUE_HPP_INCLUDED

#include <type_traits>
#include <utility>

#include <type_safe/detail/assert.hpp>

namespace type_safe
{
    /// A `Verifier` for [type_safe::constrained_value<T, Constraint, Verifier]() that `DEBUG_ASSERT`s the constraint.
    struct assertion_verifier
    {
        template <typename Value, typename Predicate>
        static void verify(const Value& val, const Predicate& p)
        {
            DEBUG_ASSERT(p(val), detail::assert_handler{}, "value does not fulfill constraint");
        }
    };

    /// A `Verifier` for [type_safe::constrained_value<T, Constraint, Verifier]() that doesn't check the constraint.
    struct null_verifier
    {
        template <typename Value, typename Predicate>
        static void verify(const Value&, const Predicate&)
        {
        }
    };

    /// A value of type `T` that always fulfills the predicate `Constraint`.
    /// The `Constraint` is checked by the `Verifier`.
    /// \requires `T` must not be a reference, `Constraint` must be a functor of type `bool(const T&)`
    /// and `Verifier` must provide a `static` function `void verify(const T&, const Predicate&)`.
    template <typename T, typename Constraint, typename Verifier = assertion_verifier>
    class constrained_value : Constraint, Verifier
    {
        static_assert(!std::is_reference<T>::value, "T must not be a reference");

    public:
        using value_type           = typename std::remove_cv<T>::type;
        using constraint_predicate = Constraint;

        /// \effects Creates it giving it a valid `value` and a `predicate`.
        /// The `value` will be copied and verified.
        explicit constrained_value(const value_type& value, constraint_predicate predicate = {})
        : Constraint(std::move(predicate)), value_(value)
        {
            verify();
        }

        /// \effects Creates it giving it a valid `value` and a `predicate`.
        /// The `value` will be moved and verified.
        explicit constrained_value(value_type&& value, constraint_predicate predicate = {})
        : Constraint(std::move(predicate)), value_(std::move(value))
        {
            verify();
        }

        /// A proxy class to provide write access to the stored value.
        /// The destructor will verify the value again.
        class modifier
        {
        public:
            /// \effects Move constructs it.
            /// `other` will not verify any value afterwards.
            modifier(modifier&& other) noexcept : value_(other.value_)
            {
                other.value_ = nullptr;
            }

            /// \effects Verifies the value, if there is any.
            ~modifier() noexcept(false)
            {
                if (value_)
                    value_->verify();
            }

            /// \effects Move assigns it.
            /// `other` will not verify any value afterwards.
            modifier& operator=(modifier&& other) noexcept
            {
                value_       = other.value_;
                other.value_ = nullptr;
                return *this;
            }

            /// \returns A reference to the stored value.
            /// \requires It must not be in the moved-from state.
            value_type& get() noexcept
            {
                DEBUG_ASSERT(value_, detail::assert_handler{});
                return value_->value_;
            }

        private:
            modifier(constrained_value& value) noexcept : value_(&value)
            {
            }

            constrained_value* value_;
            friend constrained_value;
        };

        /// \returns A proxy object to provide verified write-access to the stored value.
        modifier modify() noexcept
        {
            return modifier(*this);
        }

        /// \effects Moves the stored value out of the `constrained_value`,
        /// it will not be checked further.
        /// \returns An rvalue reference to the stored value.
        value_type&& release() noexcept
        {
            return std::move(value_);
        }

        /// \returns A `const` reference to the stored value.
        /// \requires Any `const` operations on the `value_type` must not affect the validity of the value.
        const value_type& get_value() const noexcept
        {
            return value_;
        }

        /// \returns The predicate that determines validity.
        const constraint_predicate& get_constraint() const noexcept
        {
            return *this;
        }

    private:
        void verify() const
        {
            Verifier::verify(value_, get_constraint());
        }

        value_type value_;
    };

    /// \effects Calls `f` with a non-`const` reference to the stored value of the [type_safe::constrained_value<T, Constraint, Verifier>]().
    /// It checks that `f` does not change the validity of the object.
    /// \notes The same behavior can be accomplished by using the `modify()` member function.
    template <typename T, typename Constraint, class Verifier, typename Func>
    void with(constrained_value<T, Constraint, Verifier>& value, Func&& f)
    {
        auto modifier = value.modify();
        std::forward<Func>(f)(modifier.get());
    }

    namespace constraints
    {
        /// A `Constraint` for the [type_safe::constrained_value<T, Constraint, Verifier>]().
        /// A value of a pointer type is valid if it is not equal to `nullptr`.
        struct non_null
        {
            template <typename T>
            bool operator()(const T* ptr) const noexcept
            {
                return ptr != nullptr;
            }
        };

        /// A `Constraint` for the [type_safe::constrained_value<T, Constraint, Verifier>]().
        /// A value of a container type is valid if it is not empty.
        /// Empty-ness is determined with either a member or non-member function.
        struct non_empty
        {
            template <typename T>
            auto operator()(const T& t) const noexcept(noexcept(t.empty())) -> decltype(t.empty())
            {
                return !t.empty();
            }

            template <typename T>
            auto operator()(const T& t) const noexcept(noexcept(empty(t))) -> decltype(empty(t))
            {
                return !empty(t);
            }
        };

        /// A `Constraint` for the [type_safe::constrained_value<T, Constraint, Verifier>]().
        /// A value is valid if it not equal to the default constructed value.
        struct non_default
        {
            template <typename T>
            bool operator()(const T& t) const noexcept(noexcept(t == T()))
            {
                return !(t == T());
            }
        };

        /// A `Constraint` for the [type_safe::constrained_value<T, Constraint, Verifier>]().
        /// A value of a pointer-like type is valid if the expression `!value` is `true`.
        struct non_invalid
        {
            template <typename T>
            bool operator()(const T& t) const noexcept(noexcept(!t))
            {
                return !t;
            }
        };
    } // namespace constraints
} // namespace type_safe

#endif // TYPE_SAFE_CONSTRAINED_VALUE_HPP_INCLUDED
