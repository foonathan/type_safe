// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_DETAIL_VARIANT_IMPL_HPP_INCLUDED
#define TYPE_SAFE_DETAIL_VARIANT_IMPL_HPP_INCLUDED

#include <type_safe/detail/all_of.hpp>
#include <type_safe/detail/assign_or_construct.hpp>
#include <type_safe/detail/copy_move_control.hpp>
#include <type_safe/detail/is_nothrow_swappable.hpp>
#include <type_safe/tagged_union.hpp>

namespace type_safe
{
    template <class VariantPolicy, typename Head, typename... Types>
    class basic_variant;

    namespace detail
    {
        //=== variant traits ===//
        template <typename T>
        struct is_variant_impl : std::false_type
        {
        };

        template <class VariantPolicy, typename... Types>
        struct is_variant_impl<basic_variant<VariantPolicy, Types...>> : std::true_type
        {
        };

        template <typename T>
        using is_variant = is_variant_impl<typename std::decay<T>::type>;

        template <typename... Types>
        struct traits_impl
        {
            using copy_constructible = all_of<std::is_copy_constructible<Types>::value...>;
            using move_constructible = all_of<std::is_move_constructible<Types>::value...>;
            using nothrow_move_constructible =
                all_of<std::is_nothrow_move_constructible<Types>::value...>;
            using nothrow_move_assignable =
                all_of<nothrow_move_constructible::value,
                       (std::is_move_assignable<Types>::value ?
                            std::is_nothrow_move_assignable<Types>::value :
                            true)...>;
            using nothrow_swappable =
                all_of<nothrow_move_constructible::value, is_nothrow_swappable<Types>::value...>;
        };

        template <typename... Types>
        using traits = traits_impl<typename std::decay<Types>::type...>;

        //=== copy_assign_union_value ===//
        template <class VariantPolicy, class Union>
        class copy_assign_union_value;

        template <class VariantPolicy, typename... Types>
        class copy_assign_union_value<VariantPolicy, tagged_union<Types...>>
        {
        public:
            static void assign(tagged_union<Types...>& dest, const tagged_union<Types...>& org)
            {
                auto idx = static_cast<std::size_t>(org.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), detail::assert_handler{});
                callbacks[idx](dest, org);
            }

        private:
            template <typename T,
                      typename = typename std::enable_if<std::is_copy_assignable<T>::value>::type>
            static void do_assign(union_type<T> type, tagged_union<Types...>& dest,
                                  const tagged_union<Types...>& org)
            {
                dest.value(type) = org.value(type);
            }

            template <typename T,
                      typename std::enable_if<!std::is_copy_assignable<T>::value, int>::type = 0>
            static void do_assign(union_type<T> type, tagged_union<Types...>& dest,
                                  const tagged_union<Types...>& org)
            {
                VariantPolicy::change_value(type, dest, org.value(type));
            }

            template <typename T>
            static void assign_impl(tagged_union<Types...>& dest, const tagged_union<Types...>& org)
            {
                constexpr auto id = typename tagged_union<Types...>::type_id(union_type<T>{});
                DEBUG_ASSERT(org.type() == id, detail::assert_handler{});

                if (dest.type() == id)
                    do_assign(union_type<T>{}, dest, org);
                else
                    VariantPolicy::change_value(union_type<T>{}, dest, org.value(union_type<T>{}));
            }

            using callback_type = void (*)(tagged_union<Types...>&, const tagged_union<Types...>&);
            static constexpr callback_type callbacks[] = {&assign_impl<Types>...};
        };
        template <class VariantPolicy, typename... Types>
        constexpr
            typename copy_assign_union_value<VariantPolicy, tagged_union<Types...>>::callback_type
                copy_assign_union_value<VariantPolicy, tagged_union<Types...>>::callbacks[];

        //==== move_assign_union_value ===//
        template <class VariantPolicy, class Union>
        class move_assign_union_value;

        template <class VariantPolicy, typename... Types>
        class move_assign_union_value<VariantPolicy, tagged_union<Types...>>
        {
        public:
            static void assign(tagged_union<Types...>& dest, tagged_union<Types...>&& org)
            {
                auto idx = static_cast<std::size_t>(org.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), detail::assert_handler{});
                callbacks[idx](dest, std::move(org));
            }

        private:
            template <typename T,
                      typename = typename std::enable_if<std::is_move_assignable<T>::value>::type>
            static void do_assign(union_type<T> type, tagged_union<Types...>& dest,
                                  tagged_union<Types...>&& org)
            {
                dest.value(type) = std::move(org).value(type);
            }

            template <typename T,
                      typename std::enable_if<!std::is_move_assignable<T>::value, int>::type = 0>
            static void do_assign(union_type<T> type, tagged_union<Types...>& dest,
                                  tagged_union<Types...>&& org)
            {
                VariantPolicy::change_value(type, dest, std::move(org).value(type));
            }

            template <typename T>
            static void assign_impl(tagged_union<Types...>& dest, tagged_union<Types...>&& org)
            {
                constexpr auto id = typename tagged_union<Types...>::type_id(union_type<T>{});
                DEBUG_ASSERT(org.type() == id, detail::assert_handler{});

                if (dest.type() == id)
                    do_assign(union_type<T>{}, dest, std::move(org));
                else
                    VariantPolicy::change_value(union_type<T>{}, dest,
                                                std::move(org).value(union_type<T>{}));
            }

            using callback_type = void (*)(tagged_union<Types...>&, tagged_union<Types...>&&);
            static constexpr callback_type callbacks[] = {&assign_impl<Types>...};
        };
        template <class VariantPolicy, typename... Types>
        constexpr
            typename move_assign_union_value<VariantPolicy, tagged_union<Types...>>::callback_type
                move_assign_union_value<VariantPolicy, tagged_union<Types...>>::callbacks[];

        //=== swap_union ===//
        template <class VariantPolicy, class Union>
        class swap_union;

        template <class VariantPolicy, typename... Types>
        class swap_union<VariantPolicy, tagged_union<Types...>>
        {
        public:
            static void swap(tagged_union<Types...>& a, tagged_union<Types...>& b)
            {
                auto idx = static_cast<std::size_t>(a.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), detail::assert_handler{});
                callbacks[idx](a, b);
            }

        private:
            template <typename T>
            static void swap_impl(tagged_union<Types...>& a, tagged_union<Types...>& b)
            {
                constexpr auto id = typename tagged_union<Types...>::type_id(union_type<T>{});
                DEBUG_ASSERT(a.type() == id, detail::assert_handler{});

                if (b.type() == id)
                {
                    using std::swap;
                    swap(a.value(union_type<T>{}), b.value(union_type<T>{}));
                }
                else
                {
                    T tmp(std::move(a).value(union_type<T>{})); // save old value from a
                    // assign a to value in b
                    move_assign_union_value<VariantPolicy,
                                            tagged_union<Types...>>::assign(a, std::move(b));
                    // change value in b to tmp
                    VariantPolicy::change_value(union_type<T>{}, b, std::move(tmp));
                }
            }

            using callback_type = void (*)(tagged_union<Types...>&, tagged_union<Types...>&);
            static constexpr callback_type callbacks[] = {&swap_impl<Types>...};
        };
        template <class VariantPolicy, typename... Types>
        constexpr typename swap_union<VariantPolicy, tagged_union<Types...>>::callback_type
            swap_union<VariantPolicy, tagged_union<Types...>>::callbacks[];

        //=== map_union ===//
        template <typename Functor, class Union>
        class map_union;

        template <typename Functor, typename... Types>
        class map_union<Functor, tagged_union<Types...>>
        {
        public:
            static void map(tagged_union<Types...>& res, const tagged_union<Types...>& tunion,
                            Functor&& f)
            {
                DEBUG_ASSERT(!res.has_value(), assert_handler{});
                auto idx = static_cast<std::size_t>(tunion.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), assert_handler{});
                copy_callbacks[idx](res, tunion, std::forward<Functor>(f));
            }
            static void map(tagged_union<Types...>& res, tagged_union<Types...>&& tunion,
                            Functor&& f)
            {
                DEBUG_ASSERT(!res.has_value(), assert_handler{});
                auto idx = static_cast<std::size_t>(tunion.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), assert_handler{});
                move_callbacks[idx](res, std::move(tunion), std::forward<Functor>(f));
            }

        private:
            template <typename T, typename Result =
                                      decltype(std::declval<Functor&&>()(std::declval<const T&>()))>
            static void call(int, tagged_union<Types...>& res, const tagged_union<Types...>& tunion,
                             Functor&& f)
            {
                auto&& result = std::forward<Functor>(f)(tunion.value(union_type<T>{}));
                res.emplace(union_type<typename std::decay<Result>::type>{},
                            std::forward<Result>(result));
            }
            template <typename T,
                      typename Result = decltype(std::declval<Functor&&>()(std::declval<T&&>()))>
            static void call(int, tagged_union<Types...>& res, tagged_union<Types...>&& tunion,
                             Functor&& f)
            {
                auto&& result = std::forward<Functor>(f)(std::move(tunion).value(union_type<T>{}));
                res.emplace(union_type<typename std::decay<Result>::type>{},
                            std::forward<Result>(result));
            }

            template <typename T>
            static void call(short, tagged_union<Types...>& res,
                             const tagged_union<Types...>& tunion, Functor&&)
            {
                res.emplace(union_type<T>{}, tunion.value(union_type<T>{}));
            }
            template <typename T>
            static void call(short, tagged_union<Types...>& res, tagged_union<Types...>&& tunion,
                             Functor&&)
            {
                res.emplace(union_type<T>{}, std::move(tunion).value(union_type<T>{}));
            }

            template <typename T>
            static void map_impl_copy(tagged_union<Types...>&       res,
                                      const tagged_union<Types...>& tunion, Functor&& f)
            {
                call<T>(0, res, tunion, std::forward<Functor>(f));
            }
            template <typename T>
            static void map_impl_move(tagged_union<Types...>& res, tagged_union<Types...>&& tunion,
                                      Functor&& f)
            {
                call<T>(0, res, std::move(tunion), std::forward<Functor>(f));
            }

            using copy_callback_type = void (*)(tagged_union<Types...>&,
                                                const tagged_union<Types...>&, Functor&&);
            using move_callback_type = void (*)(tagged_union<Types...>&, tagged_union<Types...>&&,
                                                Functor&&);

            static constexpr copy_callback_type copy_callbacks[] = {&map_impl_copy<Types>...};
            static constexpr move_callback_type move_callbacks[] = {&map_impl_move<Types>...};
        };
        template <typename Functor, typename... Types>
        constexpr typename map_union<Functor, tagged_union<Types...>>::copy_callback_type
            map_union<Functor, tagged_union<Types...>>::copy_callbacks[];
        template <typename Functor, typename... Types>
        constexpr typename map_union<Functor, tagged_union<Types...>>::move_callback_type
            map_union<Functor, tagged_union<Types...>>::move_callbacks[];

        //=== compare_variant ===//
        template <class Variant>
        class compare_variant;

        template <class VariantPolicy, typename... Types>
        class compare_variant<basic_variant<VariantPolicy, Types...>>
        {
        public:
            static bool compare_equal(const basic_variant<VariantPolicy, Types...>& a,
                                      const basic_variant<VariantPolicy, Types...>& b)
            {
                if (!a.has_value())
                    // to be equal, b must not have value as well
                    return !b.has_value();
                auto idx = static_cast<std::size_t>(a.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), assert_handler{});
                return equal_callbacks[idx](a, b);
            }

            static bool compare_less(const basic_variant<VariantPolicy, Types...>& a,
                                     const basic_variant<VariantPolicy, Types...>& b)
            {
                if (!a.has_value())
                    // for a to be less than b,
                    // b must have a value
                    return b.has_value();
                auto idx = static_cast<std::size_t>(a.type()) - 1;
                DEBUG_ASSERT(idx < sizeof...(Types), assert_handler{});
                return less_callbacks[idx](a, b);
            }

        private:
            template <typename T>
            static bool compare_equal_impl(const basic_variant<VariantPolicy, Types...>& a,
                                           const basic_variant<VariantPolicy, Types...>& b)
            {
                DEBUG_ASSERT(a.has_value(union_type<T>{}), assert_handler{});
                return a.value(union_type<T>{}) == b;
            }

            template <typename T>
            static bool compare_less_impl(const basic_variant<VariantPolicy, Types...>& a,
                                          const basic_variant<VariantPolicy, Types...>& b)
            {
                DEBUG_ASSERT(a.has_value(union_type<T>{}), assert_handler{});
                return a.value(union_type<T>{}) < b;
            }

            using callback_type = bool (*)(const basic_variant<VariantPolicy, Types...>&,
                                           const basic_variant<VariantPolicy, Types...>&);
            static constexpr callback_type equal_callbacks[] = {&compare_equal_impl<Types>...};
            static constexpr callback_type less_callbacks[]  = {&compare_less_impl<Types>...};
        };
        template <class VariantPolicy, typename... Types>
        constexpr typename compare_variant<basic_variant<VariantPolicy, Types...>>::callback_type
            compare_variant<basic_variant<VariantPolicy, Types...>>::equal_callbacks[];
        template <class VariantPolicy, typename... Types>
        constexpr typename compare_variant<basic_variant<VariantPolicy, Types...>>::callback_type
            compare_variant<basic_variant<VariantPolicy, Types...>>::less_callbacks[];

        //=== with variant ===//
        template <typename Func, class Variant, class Types>
        class with_variant;

        template <typename Func, class Variant, typename... Types>
        class with_variant<Func, Variant, union_types<Types...>>
        {
        public:
            static void with(Variant&& variant, Func&& func)
            {
                if (variant.has_value())
                {
                    auto idx = static_cast<std::size_t>(variant.type()) - 1;
                    DEBUG_ASSERT(idx < sizeof...(Types), assert_handler{});
                    callbacks[idx](std::forward<Variant>(variant), std::forward<Func>(func));
                }
            }

        private:
            template <typename T>
            static auto call(int, Variant&& variant, Func&& func) -> decltype(
                std::forward<Func>(func)(std::forward<Variant>(variant).value(union_type<T>{})))
            {
                return std::forward<Func>(func)(
                    std::forward<Variant>(variant).value(union_type<T>{}));
            }

            template <typename T>
            static void call(short, Variant&&, Func&&)
            {
            }

            template <typename T>
            static void with_impl(Variant&& variant, Func&& func)
            {
                call<T>(0, std::forward<Variant>(variant), std::forward<Func>(func));
            }

            using callback_type                        = void (*)(Variant&&, Func&&);
            static constexpr callback_type callbacks[] = {&with_impl<Types>...};
        };

        template <typename Func, class Variant, typename... Types>
        constexpr typename with_variant<Func, Variant, union_types<Types...>>::callback_type
            with_variant<Func, Variant, union_types<Types...>>::callbacks[];

        //=== variant_storage ===//
        template <class VariantPolicy, typename... Types>
        class variant_storage
        {
            using traits = detail::traits<Types...>;

        public:
            variant_storage() noexcept = default;

            variant_storage(const variant_storage& other)
            {
                if (other.storage_.has_value())
                    copy(storage_, other.storage_);
            }

            variant_storage(variant_storage&& other) noexcept(
                traits::nothrow_move_constructible::value)
            {
                if (other.storage_.has_value())
                    move(storage_, std::move(other.storage_));
            }

            ~variant_storage() noexcept
            {
                if (storage_.has_value())
                    destroy(storage_);
            }

            variant_storage& operator=(const variant_storage& other)
            {
                if (storage_.has_value() && other.storage_.has_value())
                    copy_assign_union_value<VariantPolicy,
                                            tagged_union<Types...>>::assign(storage_,
                                                                            other.storage_);
                else if (storage_.has_value() && !other.storage_.has_value())
                    destroy(storage_);
                else if (!storage_.has_value() && other.storage_.has_value())
                    copy(storage_, other.storage_);

                return *this;
            }

            variant_storage& operator=(variant_storage&& other) noexcept(
                traits::nothrow_move_assignable::value)
            {
                if (storage_.has_value() && other.storage_.has_value())
                    move_assign_union_value<VariantPolicy,
                                            tagged_union<Types...>>::assign(storage_,
                                                                            std::move(
                                                                                other.storage_));
                else if (storage_.has_value() && !other.storage_.has_value())
                    destroy(storage_);
                else if (!storage_.has_value() && other.storage_.has_value())
                    copy(storage_, other.storage_);

                return *this;
            }

            tagged_union<Types...>& get_union() noexcept
            {
                return storage_;
            }

            const tagged_union<Types...>& get_union() const noexcept
            {
                return storage_;
            }

        private:
            tagged_union<Types...> storage_;
        };

        template <typename... Types>
        using variant_copy = copy_control<traits<Types...>::copy_constructible::value>;

        template <typename... Types>
        using variant_move = move_control<traits<Types...>::move_constructible::value>;

        template <class Union, typename T, typename... Args>
        using enable_variant_type_impl =
            typename std::enable_if<typename Union::type_id(union_type<T>{}) != Union::invalid_type
                                    && std::is_constructible<T, Args...>::value>::type;

        template <class Union, typename T, typename... Args>
        using enable_variant_type =
            enable_variant_type_impl<Union, typename std::decay<T>::type, Args...>;
    }
} // namespace type_safe::detail

#endif // TYPE_SAFE_DETAIL_VARIANT_IMPL_HPP_INCLUDED
