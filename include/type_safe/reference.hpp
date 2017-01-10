// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_REFERENCE_HPP_INCLUDED
#define TYPE_SAFE_REFERENCE_HPP_INCLUDED

#include <type_traits>
#include <utility>

namespace type_safe
{
    /// A reference to an object of some type `T`.
    ///
    /// Unlike [std::reference_wrapper]() it does not try to model reference semantics,
    /// instead it is basically a non-null pointer to a single object.
    /// This allows rebinding on assignment.
    /// Apart from the different access syntax it can be safely used instead of a reference,
    /// and is safe for all kinds of containers.
    ///
    /// If the given type is `const`, it will only return a `const` reference,
    /// but then `XValue` must be `false`.
    ///
    /// If `XValue` is `true`, dereferencing will [std::move()]() the object,
    /// modelling a reference to an expiring lvalue.
    /// \notes `T` is the type without the reference, ie. `object_ref<int>`.
    template <typename T, bool XValue = false>
    class object_ref
    {
        static_assert(!std::is_void<T>::value, "must not be void");
        static_assert(!std::is_reference<T>::value, "pass the type without reference");
        static_assert(!XValue || !std::is_const<T>::value, "must not be const if xvalue reference");

    public:
        using value_type     = T;
        using reference_type = typename std::conditional<XValue, T&&, T&>::type;

        /// \effects Binds the reference to the given object.
        /// \notes This constructor will only participate in overload resolution
        /// if `U` is a compatible type (i.e. non-const variant or derived type).
        /// \group ctor_assign
        /// \param 1
        /// \exclude
        template <typename U, typename = decltype(std::declval<T*&>() = std::declval<U*>())>
        explicit constexpr object_ref(U& obj) noexcept : ptr_(&obj)
        {
        }

        /// \group ctor_assign
        /// \param 1
        /// \exclude
        template <typename U, typename = decltype(std::declval<T*&>() = std::declval<U*>())>
        explicit constexpr object_ref(const object_ref<U>& obj) noexcept : ptr_(&*obj)
        {
        }

        /// \group ctor_assign
        /// \param 1
        /// \exclude
        template <typename U, typename = decltype(std::declval<T*&>() = std::declval<U*>())>
        object_ref& operator=(U& obj) noexcept
        {
            ptr_ = &obj;
            return *this;
        }

        /// \group ctor_assign
        /// \param 1
        /// \exclude
        template <typename U, typename = decltype(std::declval<T*&>() = std::declval<U*>())>
        object_ref& operator=(const object_ref<U>& obj) noexcept
        {
            ptr_ = &*obj;
            return *this;
        }

        /// \returns A native reference to the referenced object.
        /// if `XValue` is true, this will be an rvalue reference,
        /// else an lvalue reference.
        /// \group deref
        constexpr reference_type get() const noexcept
        {
            return **this;
        }

        /// \group deref
        constexpr reference_type operator*() const noexcept
        {
            return static_cast<reference_type>(*ptr_);
        }

        /// Member access operator.
        constexpr T* operator->() const noexcept
        {
            return ptr_;
        }

    private:
        T* ptr_;
    };

    /// Comparison operator for [ts::object_ref]().
    ///
    /// Two references are equal if both refer to the same object.
    /// A reference is equal to an object if the reference refers to that object.
    /// \notes These functions do not participate in overload resolution
    /// if the types are not compatible (i.e. const/non-const or derived).
    /// \group ref_compare Object reference comparison
    /// \param 3
    /// \exclude
    template <typename T, typename U, bool XValue,
              typename = decltype(std::declval<T*>() == std::declval<U*>())>
    constexpr bool operator==(const object_ref<T, XValue>& a,
                              const object_ref<U, XValue>& b) noexcept
    {
        return a.operator->() == b.operator->();
    }

    /// \group ref_compare
    /// \param 3
    /// \exclude
    template <typename T, typename U, bool XValue,
              typename = decltype(std::declval<T*>() == std::declval<U*>())>
    constexpr bool operator==(const object_ref<T, XValue>& a, U& b) noexcept
    {
        return a.operator->() == &b;
    }

    /// \group ref_compare
    /// \param 3
    /// \exclude
    template <typename T, typename U, bool XValue,
              typename = decltype(std::declval<T*>() == std::declval<U*>())>
    constexpr bool operator==(const T& a, const object_ref<U, XValue>& b) noexcept
    {
        return &a == b.operator->();
    }

    /// \group ref_compare
    /// \param 3
    /// \exclude
    template <typename T, typename U, bool XValue,
              typename = decltype(std::declval<T*>() == std::declval<U*>())>
    constexpr bool operator!=(const object_ref<T, XValue>& a,
                              const object_ref<U, XValue>& b) noexcept
    {
        return !(a == b);
    }

    /// \group ref_compare
    /// \param 3
    /// \exclude
    template <typename T, typename U, bool XValue,
              typename = decltype(std::declval<T*>() == std::declval<U*>())>
    constexpr bool operator!=(const object_ref<T, XValue>& a, U& b) noexcept
    {
        return !(a == b);
    }

    /// \group ref_compare
    /// \param 3
    /// \exclude
    template <typename T, typename U, bool XValue,
              typename = decltype(std::declval<T*>() == std::declval<U*>())>
    constexpr bool operator!=(const T& a, const object_ref<U, XValue>& b) noexcept
    {
        return !(a == b);
    }

    /// With operation for [ts::object_ref]().
    /// \effects Calls the `operator()` of `f` passing it `*ref` and the additional arguments.
    template <typename T, bool XValue, typename Func, typename... Args>
    void with(const object_ref<T, XValue>& ref, Func&& f, Args&&... additional_args)
    {
        std::forward<Func>(f)(*ref, std::forward<Args>(additional_args)...);
    }

    /// Creates a (const) [ts::object_ref]().
    /// \returns A [ts::object_ref]() to the given object.
    /// \group object_ref_ref
    template <typename T>
    constexpr object_ref<T> ref(T& obj) noexcept
    {
        return object_ref<T>(obj);
    }

    /// \group object_ref_ref
    template <typename T>
    constexpr object_ref<const T> cref(const T& obj) noexcept
    {
        return object_ref<const T>(obj);
    }

    /// Convenience alias of [ts::object_ref]() where `XValue` is `true`.
    template <typename T>
    using xvalue_ref = object_ref<T, true>;

    /// Creates a [ts::xvalue_ref]().
    /// \returns A [ts::xvalue_ref]() to the given object.
    template <typename T>
    constexpr xvalue_ref<T> xref(T& obj) noexcept
    {
        return xvalue_ref<T>(obj);
    }

    /// \returns A new object containing a copy of the referenced object.
    /// It will use the copy (1)/move constructor (2).
    /// \throws Anything thrown by the copy (1)/move (2) constructor.
    /// \group object_ref_copy
    template <typename T>
    constexpr typename std::remove_const<T>::type copy(const object_ref<T>& obj)
    {
        return *obj;
    }

    /// \group object_ref_copy
    template <typename T>
    constexpr T move(const xvalue_ref<T>& obj) noexcept(
        std::is_nothrow_move_constructible<T>::value)
    {
        return *obj;
    }
} // namespace type_safe

#endif // TYPE_SAFE_REFERENCE_HPP_INCLUDED
