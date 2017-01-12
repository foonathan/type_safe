// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_REFERENCE_HPP_INCLUDED
#define TYPE_SAFE_REFERENCE_HPP_INCLUDED

#include <type_traits>
#include <utility>

#include <type_safe/detail/assert.hpp>
#include <type_safe/index.hpp>

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

    /// A reference to an array of objects of type `T`.
    ///
    /// It is a simple pointer + size pair that allows reference access to each element in the array.
    /// An "array" here is any continguous storage (so C arrays, [std::vector](), etc.).
    /// It does not allow changing the size of the array, only the individual elements.
    /// Like [ts::object_ref]() it can be safely used in containers.
    ///
    /// If the given type is `const`, it will only return a `const` reference to each elment,
    /// but then `XValue` must be `false`.
    ///
    /// If `XValue` is `true`, dereferencing will [std::move()]() the object,
    /// modelling a reference to an expiring lvalue.
    /// \notes `T` is the type stored in the array, so `array_ref<int>` to reference a contigous storage of `int`s.
    template <typename T, bool XValue = false>
    class array_ref
    {
        static_assert(!std::is_void<T>::value, "must not be void");
        static_assert(!std::is_reference<T>::value, "pass the type without reference");
        static_assert(!XValue || !std::is_const<T>::value, "must not be const if xvalue reference");

    public:
        using value_type     = T;
        using reference_type = typename std::conditional<XValue, T&&, T&>::type;
        using iterator       = T*;

        /// \effects Sets the reference to the memory range `[begin, end)`.
        /// \requires `begin` and `end` must not be `nullptr`, `begin <= end`.
        /// \group range
        array_ref(T* begin, T* end) noexcept : size_(0u)
        {
            assign(begin, end);
        }

        /// \effects Sets the reference to the memory range `[array, array + size)`.
        /// \requires `array` must not be `nullptr`.
        /// \group ptr_size
        array_ref(T* array, size_t size) noexcept : size_(size)
        {
            assign(array, size);
        }

        /// \effects Sets the reference to the C array.
        /// \group c_array
        template <std::size_t Size>
        explicit array_ref(T (&arr)[Size]) : begin_(arr), size_(Size)
        {
        }

        /// \group c_array
        template <std::size_t Size>
        array_ref& operator=(T (&arr)[Size]) noexcept
        {
            assign(arr);
            return *this;
        }

        /// \group c_array
        template <std::size_t Size>
        void                  assign(T (&arr)[Size]) noexcept
        {
            begin_ = arr;
            size_  = Size;
        }

        /// \group range
        void assign(T* begin, T* end) noexcept
        {
            DEBUG_ASSERT(begin && end && begin <= end, detail::precondition_error_handler{},
                         "invalid array bounds");
            begin_ = begin;
            size_  = static_cast<size_t>(make_unsigned(end - begin));
        }

        /// \group ptr_size
        void assign(T* array, size_t size) noexcept
        {
            DEBUG_ASSERT(array, detail::precondition_error_handler{}, "invalid array bounds");
            begin_ = array;
            size_  = size;
        }

        /// \returns An iterator to the beginning of the array.
        iterator begin() const noexcept
        {
            return begin_;
        }

        /// \returns An iterator one past the last element of the array.
        iterator end() const noexcept
        {
            return begin_ + size_.get();
        }

        /// \returns A (non-null) pointer to the beginning of the array.
        T* data() const noexcept
        {
            return begin_;
        }

        /// \returns The number of elements in the array.
        size_t size() const noexcept
        {
            return size_;
        }

        /// \returns A (`rvalue` if `Xvalue` is `true`) reference to the `i`th element of the array.
        /// \requires `i < size()`.
        reference_type operator[](index_t i) const noexcept
        {
            DEBUG_ASSERT(static_cast<size_t&>(i) < size_, detail::precondition_error_handler{},
                         "out of bounds array access");
            return static_cast<reference_type>(at(begin_, i));
        }

    private:
        T*     begin_;
        size_t size_;
    };

    /// With operation for [ts::array_ref]().
    /// \effects Calls the `operator()` of `f` passing it `ref[i]` and the additional arguments for each valid index of the array.
    template <typename T, bool XValue, typename Func, typename... Args>
    void with(const array_ref<T, XValue>& ref, Func& f, Args&&... additional_args)
    {
        for (auto&& elem : ref)
            f(elem, std::forward<Args>(additional_args)...);
    }

    /// Creates a [ts::array_ref]().
    /// \returns The reference created by forwarding the parameter(s) to the constructor.
    /// \group array_ref_ref
    template <typename T, std::size_t Size>
    array_ref<T> ref(T (&arr)[Size]) noexcept
    {
        return array_ref<T>(arr);
    }

    /// \group array_ref_ref
    template <typename T>
    array_ref<T> ref(T* begin, T* end) noexcept
    {
        return array_ref<T>(begin, end);
    }

    /// \group array_ref_ref
    template <typename T>
    array_ref<T> ref(T* array, size_t size) noexcept
    {
        return array_ref<T>(array, size);
    }

    /// Creates a [ts::array_ref]() to `const`.
    /// \returns The reference created by forwarding the parameter(s) to the constructor.
    /// \group array_ref_cref
    template <typename T, std::size_t Size>
    array_ref<const T> cref(const T (&arr)[Size]) noexcept
    {
        return array_ref<const T>(arr);
    }

    /// \group array_ref_cref
    template <typename T>
    array_ref<const T> cref(const T* begin, const T* end) noexcept
    {
        return array_ref<const T>(begin, end);
    }

    /// \group array_ref_cref
    template <typename T>
    array_ref<const T> cref(const T* array, size_t size) noexcept
    {
        return array_ref<const T>(array, size);
    }

    /// Convenience alias for [ts::array_ref]() where `XValue` is `true`.
    template <typename T>
    using array_xvalue_ref = array_ref<T, true>;

    /// Creates a [ts::array_xvalue_ref]().
    /// \returns The reference created by forwarding the parameter(s) to the constructor.
    /// \group array_xvalue_ref_ref
    template <typename T, std::size_t Size>
    array_xvalue_ref<T> xref(T (&arr)[Size]) noexcept
    {
        return array_xvalue_ref<T>(arr);
    }

    /// \group array_xvalue_ref_ref
    template <typename T>
    array_xvalue_ref<T> xref(T* begin, T* end) noexcept
    {
        return array_xvalue_ref<T>(begin, end);
    }

    /// \group array_xvalue_ref_ref
    template <typename T>
    array_xvalue_ref<T> xref(T* array, size_t size) noexcept
    {
        return array_xvalue_ref<T>(array, size);
    }
} // namespace type_safe

#endif // TYPE_SAFE_REFERENCE_HPP_INCLUDED
