// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_OPTIONAL_REF_HPP_INCLUDED
#define TYPE_SAFE_OPTIONAL_REF_HPP_INCLUDED

#include <type_safe/optional.hpp>

namespace type_safe
{
    /// \exclude
    namespace detail
    {
        template <typename T>
        T& move_if(std::false_type, T& obj) noexcept
        {
            return obj;
        }

        template <typename T>
        T&& move_if(std::true_type, T& obj) noexcept
        {
            return std::move(obj);
        }
    }

    /// A `StoragePolicy` for [ts::basic_optional]() that allows optional references.
    ///
    /// The actual `value_type` passed to the optional is [std::reference_wrapper<T>](),
    /// but the reference types are normal references, so `value()` will return a `T&`
    /// and `value_or()` takes a fallback reference of the same type and returns one of them.
    /// Assigning an optional will always change the target of the reference.
    /// You cannot pass rvalues.
    ///
    /// If `XValue` is `true`, you still cannot pass rvalues,
    /// but the result of `value()`/`value_or()` will return an rvalue reference,
    /// to allow moving of the stored value into something else.
    ///
    /// Depending on the const-ness of `T` is the reference to `const` or non-const as well,
    /// unless `XValue` is true`, in which case `T` must not be `const`.
    /// \module optional
    template <typename T, bool XValue = false>
    class reference_optional_storage
    {
        static_assert(!std::is_reference<T>::value, "pass the type without reference");
        static_assert(!XValue || !std::is_const<T>::value, "must not be const if xvalue reference");

        using result_type = typename std::conditional<XValue, T&&, T&>::type;

        struct prevent_rvalues
        {
        };

    public:
        using value_type             = std::reference_wrapper<T>;
        using lvalue_reference       = T&;
        using const_lvalue_reference = lvalue_reference;
        /// \exclude target
        using rvalue_reference       = prevent_rvalues;
        using const_rvalue_reference = rvalue_reference;

        template <typename U>
        using rebind = reference_optional_storage<U, XValue>;

        /// \effects Creates it without a bound reference.
        reference_optional_storage() noexcept : pointer_(nullptr)
        {
        }

        /// \effects Binds the reference to `obj`.
        void create_value(lvalue_reference obj) noexcept
        {
            pointer_ = &obj;
        }

        /// \effects Binds the reference to the same reference in `other`.
        void create_value(const reference_optional_storage& other) noexcept
        {
            pointer_ = other.pointer_;
        }

        /// \effects Binds the same target as `const_ref`.
        /// \param 1
        /// \exclude
        template <typename U,
                  typename = typename std::
                      enable_if<std::is_same<U, typename std::remove_const<T>::type>::value>::type>
        void create_value(const basic_optional<reference_optional_storage<U, XValue>>& const_ref)
        {
            pointer_ = const_ref.has_value() ? &const_ref.value() : nullptr;
        }

        /// \effects Same as `destroy_value()`.
        void create_value(std::nullptr_t) noexcept
        {
            destroy_value();
        }

        void create_value(T&&) = delete;

        /// \effects Binds the reference to the same reference in `other`.
        void copy_value(const reference_optional_storage& other) noexcept
        {
            pointer_ = other.pointer_;
        }

        /// \effects Swaps the reference with the reference in `other`,
        /// i.e. rebinds them, no value change.
        void swap_value(reference_optional_storage& other) noexcept
        {
            std::swap(pointer_, other.pointer_);
        }

        /// \effects Unbinds the reference.
        void destroy_value() noexcept
        {
            pointer_ = nullptr;
        }

        /// \returns `true` if the reference is bound, `false` otherwise.
        bool has_value() const noexcept
        {
            return pointer_ != nullptr;
        }

        /// \returns The target of the reference.
        /// Depending on `XValue`, this will either be `T&` or `T&&`.
        /// \exclude return
        result_type get_value() const noexcept
        {
            return static_cast<result_type>(*pointer_);
        }

        /// \returns Either `get_value()` or `other`.
        /// This must be given an lvalue of type `T` and it returns either an lvalue or an rvalue,
        /// depending on `XValue`.
        /// \exclude return
        result_type get_value_or(lvalue_reference other) const noexcept
        {
            if (has_value())
                return get_value();
            return detail::move_if(std::integral_constant<bool, XValue>{}, other);
        }

        /// \returns Either `get_value()` or `other`.
        /// The type of `other` must not be an lvalue of type `T`,
        /// it will return a new `T` object created from `other` or by copying/moving `get_value()`.
        /// \notes This function does not participate in overload resolution,
        /// unless `T` is convertible from `U`.
        /// \param 1
        /// \exclude
        template <
            typename U,
            typename =
                typename std::enable_if<!(std::is_reference<U>::value
                                          && std::is_same<typename std::remove_reference<U>::type&,
                                                          lvalue_reference>::value)
                                        && std::is_convertible<U&&, T>::value>::type>
        T get_value_or(U&& other) const
        {
            return has_value() ? get_value() : std::forward<U>(other);
        }

    private:
        T* pointer_;
    };

    /// A [ts::basic_optional]() that uses [ts::reference_optional_storage]().
    /// It is an optional reference.
    /// \notes `T` is the type without the reference, i.e. `optional_ref<int>`.
    /// \module optional
    template <typename T>
    using optional_ref = basic_optional<reference_optional_storage<T>>;

    /// \returns A [ts::optional_ref<T>]() to the pointee of `ptr` or `nullopt`.
    /// \module optional
    template <typename T>
    optional_ref<T> ref(T* ptr) noexcept
    {
        return ptr ? optional_ref<T>(*ptr) : nullopt;
    }

    /// \returns A [ts::optional_ref<T>]() to `obj`.
    /// \module optional
    /// \param 1
    /// \exclude
    template <typename T, typename = typename std::enable_if<!std::is_pointer<T>::value>::type>
    optional_ref<T> ref(T& obj) noexcept
    {
        return optional_ref<T>(obj);
    }

    /// \returns A [ts::optional_ref<T>]() to `const` to the pointee of `ptr` or `nullopt`.
    /// \module optional
    template <typename T>
    optional_ref<const T> cref(const T* ptr) noexcept
    {
        return ptr ? optional_ref<const T>(*ptr) : nullopt;
    }

    /// \returns A [ts::optional_ref<T>]() to `obj`.
    /// \module optional
    /// \param 1
    /// \exclude
    template <typename T, typename = typename std::enable_if<!std::is_pointer<T>::value>::type>
    optional_ref<const T> cref(const T& obj) noexcept
    {
        return optional_ref<const T>(obj);
    }

    /// A [ts::basic_optional]() that uses [ts::reference_optional_storage]() with `XValue` being `true`.
    /// It is an optional reference to an xvalue,
    /// i.e. an lvalue that can be moved from, like returned by `std::move()`.
    /// \notes `T` is the type without the reference, i.e. `optional_xvalue_ref<int>`.
    /// \module optional
    template <typename T>
    using optional_xvalue_ref = basic_optional<reference_optional_storage<T, true>>;

    /// \returns A [ts::optional_xvalue_ref<T>]() to the pointee of `ptr` or `nullopt`.
    /// \notes The pointee will be moved from when you call `value()`.
    /// \module optional
    template <typename T>
    optional_xvalue_ref<T> xref(T* ptr) noexcept
    {
        return ptr ? optional_xvalue_ref<T>(*ptr) : nullopt;
    }

    /// \returns A [ts::optional_xvalue_ref<T>]() to `obj`.
    /// \notes The object will be moved from when you call `value()`.
    /// \module optional
    /// \param 1
    /// \exclude
    template <typename T, typename = typename std::enable_if<!std::is_pointer<T>::value>::type>
    optional_xvalue_ref<T> xref(T& obj) noexcept
    {
        return optional_xvalue_ref<T>(obj);
    }

    /// \returns A [ts::optional<T>]() containing a copy of the value of `ref`
    /// if there is any value.
    /// \requires `T` must be copyable.
    /// \module optional
    template <typename T>
    optional<typename std::remove_const<T>::type> copy(const optional_ref<T>& ref)
    {
        return ref.has_value() ? make_optional(ref.value()) : nullopt;
    }

    /// \returns A [ts::optional<T>]() containing a copy of the value of `ref` created by move constructing
    /// if there is any value.
    /// \requires `T` must be moveable.
    /// \module optional
    template <typename T>
    optional<T> move(const optional_xvalue_ref<T>& ref) noexcept(
        std::is_nothrow_move_constructible<T>::value)
    {
        return ref.has_value() ? make_optional(ref.value()) : nullopt;
    }
} // namespace type_safe

#endif // TYPE_SAFE_OPTIONAL_REF_HPP_INCLUDED
