// Copyright (C) 2016-2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_STRONG_TYPEDEF_HPP_INCLUDED
#define TYPE_SAFE_STRONG_TYPEDEF_HPP_INCLUDED

#include <iosfwd>
#include <iterator>
#include <type_traits>
#include <utility>

#include <type_safe/detail/all_of.hpp>
#include <type_safe/config.hpp>

namespace type_safe
{
    /// A strong typedef emulation.
    ///
    /// Unlike regular typedefs, this does create a new type and only allows explicit conversion from the underlying one.
    /// The `Tag` is used to differentiate between different strong typedefs to the same type.
    /// It is designed to be used as a base class and does not provide any operations by itself.
    /// Use the types in the `strong_typedef_op` namespace to generate operations
    /// and/or your own member functions.
    ///
    /// Example:
    /// ```cpp
    /// struct my_handle
    /// : strong_typedef<my_handle, void*>,
    ///   strong_typedef_op::equality_comparison<my_handle>
    /// {
    ///     using strong_typedef::strong_typedef;
    /// };
    ///
    /// struct my_int
    /// : strong_typedef<my_int, int>,
    ///   strong_typedef_op::integer_arithmetic<my_int>,
    ///   strong_typedef_op::equality_comparison<my_int>,
    ///   strong_typedef_op::relational_comparison<my_int>
    /// {
    ///     using strong_typedef::strong_typedef;
    /// };
    /// ```
    template <class Tag, typename T>
    class strong_typedef
    {
    public:
        /// \effects Value initializes the underlying value.
        constexpr strong_typedef() : value_() {}

        /// \effects Copy (1)/moves (2) the underlying value.
        /// \group value_ctor
        explicit constexpr strong_typedef(const T& value) : value_(value) {}

        /// \group value_ctor
        explicit constexpr strong_typedef(T&& value) noexcept(
            std::is_nothrow_move_constructible<T>::value)
        : value_(static_cast<T&&>(value)) // std::move() might not be constexpr
        {
        }

        /// \returns A reference to the stored underlying value.
        /// \group value_conv
        explicit TYPE_SAFE_CONSTEXPR14 operator T&() TYPE_SAFE_LVALUE_REF noexcept
        {
            return value_;
        }

        /// \group value_conv
        explicit constexpr operator const T&() const TYPE_SAFE_LVALUE_REF noexcept
        {
            return value_;
        }

#if TYPE_SAFE_USE_REF_QUALIFIERS
        /// \group value_conv
        explicit TYPE_SAFE_CONSTEXPR14 operator T &&() && noexcept
        {
            return std::move(value_);
        }

        /// \group value_conv
        explicit constexpr operator const T &&() const && noexcept
        {
            return std::move(value_);
        }
#endif

        friend void swap(strong_typedef& a, strong_typedef& b) noexcept
        {
            using std::swap;
            swap(static_cast<T&>(a), static_cast<T&>(b));
        }

    private:
        T value_;
    };

    /// \exclude
    namespace detail
    {
        template <class Tag, typename T>
        T underlying_type(strong_typedef<Tag, T>);
    } // namespace detail

    /// The underlying type of the [ts::strong_typedef]().
    /// \exclude target
    template <class StrongTypedef>
    using underlying_type =
        decltype(detail::underlying_type(std::declval<typename std::decay<StrongTypedef>::type>()));

    /// Accesses the underlying value.
    /// \returns A reference to the underlying value.
    /// \group strong_typedef_get
    template <class Tag, typename T>
    TYPE_SAFE_CONSTEXPR14 T& get(strong_typedef<Tag, T>& type) noexcept
    {
        return static_cast<T&>(type);
    }

    /// \group strong_typedef_get
    template <class Tag, typename T>
    constexpr const T& get(const strong_typedef<Tag, T>& type) noexcept
    {
        return static_cast<const T&>(type);
    }

    /// \group strong_typedef_get
    template <class Tag, typename T>
    TYPE_SAFE_CONSTEXPR14 T&& get(strong_typedef<Tag, T>&& type) noexcept
    {
        return static_cast<T&&>(static_cast<T&>(type));
    }

    /// \group strong_typedef_get
    template <class Tag, typename T>
    constexpr const T&& get(const strong_typedef<Tag, T>&& type) noexcept
    {
        return static_cast<const T&&>(static_cast<const T&>(type));
    }

    /// Some operations for [ts::strong_typedef]().
    ///
    /// They all generate operators forwarding to the underlying type,
    /// inherit from then in the typedef definition.
    namespace strong_typedef_op
    {
        /// \exclude
        namespace detail
        {
            template <typename From, typename To>
            using enable_if_convertible = typename std::enable_if<
                !std::is_same<typename std::decay<From>::type, To>::value
                && !std::is_base_of<To, typename std::decay<From>::type>::value
                && std::is_convertible<typename std::decay<From>::type, To>::value>::type;

            template <typename From, typename To>
            using enable_if_convertible_same = typename std::enable_if<
                std::is_convertible<typename std::decay<From>::type, To>::value>::type;

            template <typename T>
            using enable_if_same_decay =
                typename std::enable_if<std::is_same<T, typename std::decay<T>::type>::value>::type;

            template <typename...>
            struct map_to_void
            {
                using type = void;
            };

            template <typename... T>
            using void_t = typename map_to_void<T...>::type;

            template <class Tag, typename T>
            strong_typedef<Tag, T> base_conversion(strong_typedef<Tag, T>*);

            template <class StrongTypedef>
            using base = decltype(base_conversion(std::declval<StrongTypedef*>()));

            template <class Tag, typename T>
            Tag tag_conversion(strong_typedef<Tag, T>*);

            template <class StrongTypedef>
            using tag = decltype(tag_conversion(std::declval<StrongTypedef*>()));

            template <template <typename> class MixedOp, typename Other>
            Other other_conversion(MixedOp<Other>*);

            template <class StrongTypedef, template <typename> class MixedOp>
            using other = decltype(other_conversion<MixedOp>(std::declval<StrongTypedef*>()));

            template <typename StrongTypedef, typename Op, class = void>
            struct strong_typedef_type_impl : std::enable_if<false>
            {
            };

            template <typename Base, typename Derived>
            using is_unambiguously_accessible_base_of = std::is_convertible<Derived*, Base*>;

            // This alias is well-formed if the following is a hierarchy
            //      `StrongTypedef` > `Tag` > `strong_typedef<Tag, T>`
            //                              > `Op`
            // where `strong_typedef<Tag, T>` is unambiguously accessible.
            // Note: We take advantage of `base<A>` and `tag<A>` being
            //      well-formed if `A` has an unambiguously accessible base of
            //      the form `strong_typedef<Tag, T>`.
            template <typename StrongTypedef, typename Op>
            using valid_hierarchy = typename std::enable_if<
                is_unambiguously_accessible_base_of<tag<StrongTypedef>, StrongTypedef>::value
                && std::is_base_of<Op, tag<StrongTypedef>>::value
                && std::is_same<base<StrongTypedef>, base<tag<StrongTypedef>>>::value>::type;

            template <typename StrongTypedef, typename Op>
            struct strong_typedef_type_impl<StrongTypedef, Op, valid_hierarchy<StrongTypedef, Op>>
            : std::enable_if<true, tag<StrongTypedef>>
            {
            };

            template <typename StrongTypedef, typename Op>
            using strong_typedef_type =
                typename strong_typedef_type_impl<typename std::decay<StrongTypedef>::type,
                                                  Op>::type;

            template <typename T, typename U, typename Op, class = void>
            struct same_strong_typedef_type_impl : std::enable_if<false>
            {
            };

            template <typename T, typename U, typename Op>
            struct same_strong_typedef_type_impl<
                T, U, Op, void_t<strong_typedef_type<T, Op>, strong_typedef_type<U, Op>>>
            : std::enable_if<
                  std::is_same<strong_typedef_type<T, Op>, strong_typedef_type<U, Op>>::value,
                  strong_typedef_type<T, Op>>
            {
            };

            template <typename T, typename U, typename Op>
            using same_strong_typedef_type = typename same_strong_typedef_type_impl<T, U, Op>::type;

            template <class StrongTypedef>
            constexpr const underlying_type<StrongTypedef>& get_underlying(
                const StrongTypedef& type)
            {
                return get(type);
            }

            template <class StrongTypedef>
            TYPE_SAFE_CONSTEXPR14 underlying_type<StrongTypedef>& get_underlying(
                StrongTypedef& type)
            {
                return get(static_cast<StrongTypedef&>(type));
            }

            template <class StrongTypedef>
            TYPE_SAFE_CONSTEXPR14 underlying_type<StrongTypedef>&& get_underlying(
                StrongTypedef&& type)
            {
                return get(static_cast<StrongTypedef&&>(type));
            }

            // ensure constexpr
            template <class T>
            constexpr T&& forward(typename std::remove_reference<T>::type& t) noexcept
            {
                return static_cast<T&&>(t);
            }

            template <class T>
            constexpr T&& forward(typename std::remove_reference<T>::type&& t) noexcept
            {
                static_assert(!std::is_lvalue_reference<T>::value,
                              "Can not forward an rvalue as an lvalue.");
                return static_cast<T&&>(t);
            }
        } // namespace detail

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP(Op, Name, Result)                                                 \
    /** \exclude */                                                                                \
    template <typename Left, typename Right,                                                       \
              class StrongTypedef = detail::same_strong_typedef_type<Left, Right, Name>>           \
    constexpr Result operator Op(const Left& lhs, const Right& rhs)                                \
    {                                                                                              \
        return Result(                                                                             \
            detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(lhs))          \
                Op detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(rhs))); \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Left, typename Right, typename = detail::enable_if_same_decay<Left>,        \
              class StrongTypedef = detail::same_strong_typedef_type<Left, Right, Name>>           \
    constexpr Result operator Op(Left&& lhs, const Right& rhs)                                     \
    {                                                                                              \
        return Result(                                                                             \
            detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(lhs))               \
                Op detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(rhs))); \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Left, typename Right, typename = detail::enable_if_same_decay<Right>,       \
              class StrongTypedef = detail::same_strong_typedef_type<Left, Right, Name>>           \
    constexpr Result operator Op(const Left& lhs, Right&& rhs)                                     \
    {                                                                                              \
        return Result(detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(     \
            lhs)) Op detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(rhs)));    \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Left, typename Right, typename = detail::enable_if_same_decay<Left>,        \
              typename            = detail::enable_if_same_decay<Right>,                           \
              class StrongTypedef = detail::same_strong_typedef_type<Left, Right, Name>>           \
    constexpr Result operator Op(Left&& lhs, Right&& rhs)                                          \
    {                                                                                              \
        return Result(detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(          \
            lhs)) Op detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(rhs)));    \
    }                                                                                              \
    /* mixed */                                                                                    \
    /** \exclude */                                                                                \
    template <typename Left, typename Other,                                                       \
              class StrongTypedef = detail::strong_typedef_type<Left, Name>,                       \
              typename = detail::enable_if_convertible<Other&&, StrongTypedef>, int = 0>           \
    constexpr Result operator Op(const Left& lhs, Other&& rhs)                                     \
    {                                                                                              \
        return Result(detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(     \
            lhs)) Op detail::get_underlying<StrongTypedef>(detail::forward<Other>(rhs)));          \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Left, typename Other, typename = detail::enable_if_same_decay<Left>,        \
              class StrongTypedef = detail::strong_typedef_type<Left, Name>,                       \
              typename = detail::enable_if_convertible<Other&&, StrongTypedef>, int = 0>           \
    constexpr Result operator Op(Left&& lhs, Other&& rhs)                                          \
    {                                                                                              \
        return Result(detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(lhs))     \
                          Op detail::get_underlying<StrongTypedef>(detail::forward<Other>(rhs)));  \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Right, typename Other,                                                      \
              class StrongTypedef = detail::strong_typedef_type<Right, Name>,                      \
              typename = detail::enable_if_convertible<Other&&, StrongTypedef>, int = 0>           \
    constexpr Result operator Op(Other&& lhs, const Right& rhs)                                    \
    {                                                                                              \
        return Result(                                                                             \
            detail::get_underlying<StrongTypedef>(detail::forward<Other>(lhs))                     \
                Op detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(rhs))); \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Right, typename Other, typename = detail::enable_if_same_decay<Right>,      \
              class StrongTypedef = detail::strong_typedef_type<Right, Name>,                      \
              typename = detail::enable_if_convertible<Other&&, StrongTypedef>, int = 0>           \
    constexpr Result operator Op(Other&& lhs, Right&& rhs)                                         \
    {                                                                                              \
        return Result(detail::get_underlying<StrongTypedef>(detail::forward<Other>(                \
            lhs)) Op detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(rhs)));    \
    }

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP_MIXED(Op, Name, Result)                                           \
    /** \exclude */                                                                                \
    template <typename Left, typename Other, typename OtherArg = detail::other<Left, Name>,        \
              class StrongTypedef = detail::strong_typedef_type<Left, Name<OtherArg>>,             \
              typename            = detail::enable_if_convertible_same<Other&&, OtherArg>>         \
    constexpr Result operator Op(const Left& lhs, Other&& rhs)                                     \
    {                                                                                              \
        return Result(get(static_cast<const StrongTypedef&>(lhs)) Op detail::forward<Other>(rhs)); \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Left, typename Other, typename = detail::enable_if_same_decay<Left>,        \
              typename OtherArg   = detail::other<Left, Name>,                                     \
              class StrongTypedef = detail::strong_typedef_type<Left, Name<OtherArg>>,             \
              typename            = detail::enable_if_convertible_same<Other&&, OtherArg>>         \
    constexpr Result operator Op(Left&& lhs, Other&& rhs)                                          \
    {                                                                                              \
        return Result(get(static_cast<StrongTypedef&&>(lhs)) Op detail::forward<Other>(rhs));      \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Right, typename Other, typename OtherArg = detail::other<Right, Name>,      \
              class StrongTypedef = detail::strong_typedef_type<Right, Name<OtherArg>>,            \
              typename            = detail::enable_if_convertible_same<Other&&, OtherArg>>         \
    constexpr Result operator Op(Other&& lhs, const Right& rhs)                                    \
    {                                                                                              \
        return Result(detail::forward<Other>(lhs) Op get(static_cast<const StrongTypedef&>(rhs))); \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Right, typename Other, typename = detail::enable_if_same_decay<Right>,      \
              typename OtherArg   = detail::other<Right, Name>,                                    \
              class StrongTypedef = detail::strong_typedef_type<Right, Name<OtherArg>>,            \
              typename            = detail::enable_if_convertible_same<Other&&, OtherArg>>         \
    constexpr Result operator Op(Other&& lhs, Right&& rhs)                                         \
    {                                                                                              \
        return Result(detail::forward<Other>(lhs) Op get(static_cast<StrongTypedef&&>(rhs)));      \
    }

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND(Op, Name)                                                \
    /** \exclude */                                                                                \
    template <typename Left, typename Right,                                                       \
              class StrongTypedef = detail::same_strong_typedef_type<Left, Right, Name>>           \
    TYPE_SAFE_CONSTEXPR14 StrongTypedef& operator Op(Left& lhs, const Right& rhs)                  \
    {                                                                                              \
        detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&>(lhs)) Op                 \
            detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(rhs));         \
        return static_cast<StrongTypedef&>(lhs);                                                   \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Left, typename Right, typename = detail::enable_if_same_decay<Right>,       \
              class StrongTypedef = detail::same_strong_typedef_type<Left, Right, Name>>           \
    TYPE_SAFE_CONSTEXPR14 StrongTypedef& operator Op(Left& lhs, Right&& rhs)                       \
    {                                                                                              \
        detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&>(lhs)) Op                 \
            detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(rhs));              \
        return static_cast<StrongTypedef&>(lhs);                                                   \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Left, typename Right, typename = detail::enable_if_same_decay<Left>,        \
              class StrongTypedef = detail::same_strong_typedef_type<Left, Right, Name>>           \
    TYPE_SAFE_CONSTEXPR14 StrongTypedef&& operator Op(Left&& lhs, const Right& rhs)                \
    {                                                                                              \
        detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(lhs)) Op                \
            detail::get_underlying<StrongTypedef>(static_cast<const StrongTypedef&>(rhs));         \
        return static_cast<StrongTypedef&&>(lhs);                                                  \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Left, typename Right, typename = detail::enable_if_same_decay<Left>,        \
              typename            = detail::enable_if_same_decay<Right>,                           \
              class StrongTypedef = detail::same_strong_typedef_type<Left, Right, Name>>           \
    TYPE_SAFE_CONSTEXPR14 StrongTypedef&& operator Op(Left&& lhs, Right&& rhs)                     \
    {                                                                                              \
        detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(lhs)) Op                \
            detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(rhs));              \
        return static_cast<StrongTypedef&&>(lhs);                                                  \
    }                                                                                              \
    /* mixed */                                                                                    \
    /** \exclude */                                                                                \
    template <typename Left, typename Other,                                                       \
              class StrongTypedef = detail::strong_typedef_type<Left, Name>,                       \
              typename = detail::enable_if_convertible<Other&&, StrongTypedef>, int = 0>           \
    TYPE_SAFE_CONSTEXPR14 StrongTypedef& operator Op(Left& lhs, Other&& rhs)                       \
    {                                                                                              \
        detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&>(lhs))                    \
            Op detail::get_underlying<StrongTypedef>(detail::forward<Other>(rhs));                 \
        return static_cast<StrongTypedef&>(lhs);                                                   \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Left, typename Other, typename = detail::enable_if_same_decay<Left>,        \
              class StrongTypedef = detail::strong_typedef_type<Left, Name>,                       \
              typename = detail::enable_if_convertible<Other&&, StrongTypedef>, int = 0>           \
    TYPE_SAFE_CONSTEXPR14 StrongTypedef&& operator Op(Left&& lhs, Other&& rhs)                     \
    {                                                                                              \
        detail::get_underlying<StrongTypedef>(static_cast<StrongTypedef&&>(lhs))                   \
            Op detail::get_underlying<StrongTypedef>(detail::forward<Other>(rhs));                 \
        return static_cast<StrongTypedef&&>(lhs);                                                  \
    }

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND_MIXED(Op, Name)                                          \
    /** \exclude */                                                                                \
    template <typename Left, typename Other, typename OtherArg = detail::other<Left, Name>,        \
              class StrongTypedef = detail::strong_typedef_type<Left, Name<OtherArg>>,             \
              typename            = detail::enable_if_convertible_same<Other&&, OtherArg>>         \
    TYPE_SAFE_CONSTEXPR14 StrongTypedef& operator Op(Left& lhs, Other&& rhs)                       \
    {                                                                                              \
        get(static_cast<StrongTypedef&>(lhs)) Op detail::forward<Other>(rhs);                      \
        return static_cast<StrongTypedef&>(lhs);                                                   \
    }                                                                                              \
    /** \exclude */                                                                                \
    template <typename Left, typename Other, typename = detail::enable_if_same_decay<Left>,        \
              typename OtherArg   = detail::other<Left, Name>,                                     \
              class StrongTypedef = detail::strong_typedef_type<Left, Name<OtherArg>>,             \
              typename            = detail::enable_if_convertible_same<Other&&, OtherArg>>         \
    TYPE_SAFE_CONSTEXPR14 StrongTypedef&& operator Op(Left&& lhs, Other&& rhs)                     \
    {                                                                                              \
        get(static_cast<StrongTypedef&&>(lhs)) Op detail::forward<Other>(rhs);                     \
        return static_cast<StrongTypedef&&>(lhs);                                                  \
    }

/// \exclude
#define TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(Name, Op)                                          \
    struct Name                                                                                    \
    {                                                                                              \
    };                                                                                             \
    TYPE_SAFE_DETAIL_MAKE_OP(Op, Name, StrongTypedef)                                              \
    TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND(Op## =, Name)                                                \
    template <typename Other>                                                                      \
    struct mixed_##Name                                                                            \
    {                                                                                              \
    };                                                                                             \
    TYPE_SAFE_DETAIL_MAKE_OP_MIXED(Op, mixed_##Name, StrongTypedef)                                \
    TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND_MIXED(Op## =, mixed_##Name)

        struct equality_comparison
        {
        };
        TYPE_SAFE_DETAIL_MAKE_OP(==, equality_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP(!=, equality_comparison, bool)

        template <typename Other>
        struct mixed_equality_comparison
        {
        };
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(==, mixed_equality_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(!=, mixed_equality_comparison, bool)

        struct relational_comparison
        {
        };
        TYPE_SAFE_DETAIL_MAKE_OP(<, relational_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP(<=, relational_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP(>, relational_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP(>=, relational_comparison, bool)

        template <typename Other>
        struct mixed_relational_comparison
        {
        };
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(<, mixed_relational_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(<=, mixed_relational_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(>, mixed_relational_comparison, bool)
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(>=, mixed_relational_comparison, bool)

        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(addition, +)
        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(subtraction, -)
        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(multiplication, *)
        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(division, /)
        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(modulo, %)

        struct increment
        {
        };

        /// \exclude
        template <typename Left, class StrongTypedef = detail::strong_typedef_type<Left, increment>>
        TYPE_SAFE_CONSTEXPR14 StrongTypedef& operator++(Left& lhs)
        {
            using type = underlying_type<StrongTypedef>;
            ++static_cast<type&>(static_cast<StrongTypedef&>(lhs));
            return static_cast<StrongTypedef&>(lhs);
        }
        /// \exclude
        template <typename Left, class StrongTypedef = detail::strong_typedef_type<Left, increment>>
        TYPE_SAFE_CONSTEXPR14 StrongTypedef operator++(Left& lhs, int)
        {
            auto result = static_cast<StrongTypedef&>(lhs);
            ++lhs;
            return result;
        }

        struct decrement
        {
        };

        /// \exclude
        template <typename Left, class StrongTypedef = detail::strong_typedef_type<Left, decrement>>
        TYPE_SAFE_CONSTEXPR14 StrongTypedef& operator--(Left& lhs)
        {
            using type = underlying_type<StrongTypedef>;
            --static_cast<type&>(static_cast<StrongTypedef&>(lhs));
            return static_cast<StrongTypedef&>(lhs);
        }
        /// \exclude
        template <typename Left, class StrongTypedef = detail::strong_typedef_type<Left, decrement>>
        TYPE_SAFE_CONSTEXPR14 StrongTypedef operator--(Left& lhs, int)
        {
            auto result = static_cast<StrongTypedef&>(lhs);
            --lhs;
            return result;
        }

        struct unary_plus
        {
        };

        /// \exclude
        template <typename Left,
                  class StrongTypedef = detail::strong_typedef_type<Left, unary_plus>>
        constexpr StrongTypedef operator+(const Left& lhs)
        {
            return StrongTypedef(+get(static_cast<const StrongTypedef&>(lhs)));
        }
        /// \exclude
        template <typename Left, typename = detail::enable_if_same_decay<Left>,
                  class StrongTypedef = detail::strong_typedef_type<Left, unary_plus>>
        constexpr StrongTypedef operator+(Left&& lhs)
        {
            return StrongTypedef(+get(static_cast<StrongTypedef&&>(lhs)));
        }

        struct unary_minus
        {
        };

        template <typename Left,
                  class StrongTypedef = detail::strong_typedef_type<Left, unary_minus>>
        constexpr StrongTypedef operator-(const Left& lhs)
        {
            return StrongTypedef(-get(static_cast<const StrongTypedef&>(lhs)));
        }
        /// \exclude
        template <typename Left, typename = detail::enable_if_same_decay<Left>,
                  class StrongTypedef = detail::strong_typedef_type<Left, unary_minus>>
        constexpr StrongTypedef operator-(Left&& lhs)
        {
            return StrongTypedef(-get(static_cast<StrongTypedef&&>(lhs)));
        }

        struct integer_arithmetic : unary_plus,
                                    unary_minus,
                                    addition,
                                    subtraction,
                                    multiplication,
                                    division,
                                    modulo,
                                    increment,
                                    decrement
        {
        };

        struct floating_point_arithmetic : unary_plus,
                                           unary_minus,
                                           addition,
                                           subtraction,
                                           multiplication,
                                           division
        {
        };

        struct complement
        {
        };

        /// \exclude
        template <typename Left,
                  class StrongTypedef = detail::strong_typedef_type<Left, complement>>
        constexpr StrongTypedef operator~(const Left& lhs)
        {
            return StrongTypedef(~get(static_cast<const StrongTypedef&>(lhs)));
        }
        /// \exclude
        template <typename Left, typename = detail::enable_if_same_decay<Left>,
                  class StrongTypedef = detail::strong_typedef_type<Left, complement>>
        constexpr StrongTypedef operator~(Left&& lhs)
        {
            return StrongTypedef(~get(static_cast<StrongTypedef&&>(lhs)));
        }

        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(bitwise_or, |)
        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(bitwise_xor, ^)
        TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP(bitwise_and, &)

        struct bitmask : complement, bitwise_or, bitwise_xor, bitwise_and
        {
        };

        template <typename IntT>
        struct bitshift
        {
        };
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(<<, bitshift, StrongTypedef)
        TYPE_SAFE_DETAIL_MAKE_OP_MIXED(>>, bitshift, StrongTypedef)
        TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND_MIXED(<<=, bitshift)
        TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND_MIXED(>>=, bitshift)

        template <class StrongTypedef, typename Result, typename ResultPtr = Result*,
                  typename ResultConstPtr = const Result*>
        struct dereference
        {
            /// \exclude
            Result& operator*()
            {
                using type = underlying_type<StrongTypedef>;
                return *static_cast<type&>(static_cast<StrongTypedef&>(*this));
            }

            /// \exclude
            const Result& operator*() const
            {
                using type = underlying_type<StrongTypedef>;
                return *static_cast<const type&>(static_cast<const StrongTypedef&>(*this));
            }

            /// \exclude
            ResultPtr operator->()
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<type&>(static_cast<StrongTypedef&>(*this));
            }

            /// \exclude
            ResultConstPtr operator->() const
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(static_cast<const StrongTypedef&>(*this));
            }
        };

        template <class StrongTypedef, typename Result, typename Index = std::size_t>
        struct array_subscript
        {
            /// \exclude
            Result& operator[](const Index& i)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<type&>(static_cast<StrongTypedef&>(*this))[i];
            }

            /// \exclude
            const Result& operator[](const Index& i) const
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(static_cast<const StrongTypedef&>(*this))[i];
            }
        };

        template <class StrongTypedef, class Category, typename T,
                  typename Distance = std::ptrdiff_t>
        struct iterator : dereference<StrongTypedef, T, T*, const T*>, increment
        {
            using iterator_category = Category;
            using value_type        = T;
            using distance_type     = Distance;
            using pointer           = T*;
            using reference         = T&;
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct input_iterator : iterator<StrongTypedef, std::input_iterator_tag, T, Distance>,
                                equality_comparison
        {
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct output_iterator : iterator<StrongTypedef, std::output_iterator_tag, T, Distance>
        {
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct forward_iterator : input_iterator<StrongTypedef, T, Distance>
        {
            using iterator_category = std::forward_iterator_tag;
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct bidirectional_iterator : forward_iterator<StrongTypedef, T, Distance>, decrement
        {
            using iterator_category = std::bidirectional_iterator_tag;
        };

        template <class StrongTypedef, typename T, typename Distance = std::ptrdiff_t>
        struct random_access_iterator : bidirectional_iterator<StrongTypedef, T, Distance>,
                                        array_subscript<StrongTypedef, T, Distance>,
                                        relational_comparison
        {
            using iterator_category = std::random_access_iterator_tag;

            /// \exclude
            StrongTypedef& operator+=(const Distance& d)
            {
                using type = underlying_type<StrongTypedef>;
                static_cast<type&>(static_cast<StrongTypedef&>(*this)) += d;
                return static_cast<StrongTypedef&>(*this);
            }

            /// \exclude
            StrongTypedef& operator-=(const Distance& d)
            {
                using type = underlying_type<StrongTypedef>;
                static_cast<type&>(static_cast<StrongTypedef&>(*this)) -= d;
                return static_cast<StrongTypedef&>(*this);
            }

            /// \exclude
            friend StrongTypedef operator+(const StrongTypedef& iter, const Distance& n)
            {
                using type = underlying_type<StrongTypedef>;
                return StrongTypedef(static_cast<const type&>(iter) + n);
            }

            /// \exclude
            friend StrongTypedef operator+(const Distance& n, const StrongTypedef& iter)
            {
                return iter + n;
            }

            /// \exclude
            friend StrongTypedef operator-(const StrongTypedef& iter, const Distance& n)
            {
                using type = underlying_type<StrongTypedef>;
                return StrongTypedef(static_cast<const type&>(iter) - n);
            }

            /// \exclude
            friend Distance operator-(const StrongTypedef& lhs, const StrongTypedef& rhs)
            {
                using type = underlying_type<StrongTypedef>;
                return static_cast<const type&>(lhs) - static_cast<const type&>(rhs);
            }
        };

        template <class StrongTypedef>
        struct input_operator
        {
            /// \exclude
            template <typename Char, class CharTraits>
            friend std::basic_istream<Char, CharTraits>& operator>>(
                std::basic_istream<Char, CharTraits>& in, StrongTypedef& val)
            {
                using type = underlying_type<StrongTypedef>;
                return in >> static_cast<type&>(val);
            }
        };

        template <class StrongTypedef>
        struct output_operator
        {
            /// \exclude
            template <typename Char, class CharTraits>
            friend std::basic_ostream<Char, CharTraits>& operator<<(
                std::basic_ostream<Char, CharTraits>& out, const StrongTypedef& val)
            {
                using type = underlying_type<StrongTypedef>;
                return out << static_cast<const type&>(val);
            }
        };

#undef TYPE_SAFE_DETAIL_MAKE_OP
#undef TYPE_SAFE_DETAIL_MAKE_OP_MIXED
#undef TYPE_SAFE_DETAIL_MAKE_OP_COMPOUND
#undef TYPE_SAFE_DETAIL_MAKE_STRONG_TYPEDEF_OP
    } // namespace strong_typedef_op
} // namespace type_safe

#endif // TYPE_SAFE_STRONG_TYPEDEF_HPP_INCLUDED
