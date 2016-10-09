// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/constrained_type.hpp>

#include <catch.hpp>

using namespace type_safe;

struct test_verifier
{
    static bool expected;

    template <typename T, typename Predicate>
    static void verify(const T& value, const Predicate& p)
    {
        REQUIRE(p(value) == expected);
    }
};

bool test_verifier::expected;

TEST_CASE("constrained_type")
{
    struct predicate
    {
        bool operator()(int i) const
        {
            return i != -1;
        }
    };

    using my_int = constrained_type<int, predicate, test_verifier>;

    SECTION("constructor")
    {
        test_verifier::expected = true;
        my_int a(5);
        REQUIRE(a.get_value() == 5);
        my_int b(-4);
        REQUIRE(b.get_value() == -4);

        test_verifier::expected = false;
        my_int c(-1);
        REQUIRE(c.get_value() == -1);
    }
    SECTION("assignment")
    {
        test_verifier::expected = true;
        my_int a(5);
        a = 2;
        a = 3;
        a = -2;

        test_verifier::expected = false;
        a                       = -1;
    }
    SECTION("modify()")
    {
        // with() is the same
        test_verifier::expected = true;
        my_int a(4);
        {
            auto modify = a.modify();
            modify.get() += 4;
        }
        REQUIRE(a.get_value() == 8);
        {
            auto modify = a.modify();
            modify.get() -= 5;
            modify.get() = 2;
        }
        REQUIRE(a.get_value() == 2);

        test_verifier::expected = false;
        {
            auto modify  = a.modify();
            modify.get() = -1;
        }
        REQUIRE(a.get_value() == -1);
        try
        {
            auto modify  = a.modify();
            modify.get() = -1;
            throw 0;
        }
        catch (...)
        {
            REQUIRE(a.get_value() == -1);
        }
    }
}

TEST_CASE("constraints::non_null")
{
    // conversion checks
    using ptr = constrained_type<int*, constraints::non_null>;
    static_assert(std::is_constructible<ptr, int*>::value, "");
    static_assert(!std::is_constructible<ptr, std::nullptr_t>::value, "");
    static_assert(std::is_assignable<ptr, int*>::value, "");
    static_assert(!std::is_assignable<ptr, std::nullptr_t>::value, "");

    constraints::non_null p;
    REQUIRE(!p(static_cast<int*>(nullptr)));
    REQUIRE(!p(static_cast<int*>(0)));

    int a;
    REQUIRE(p(&a));
}

struct my_container
{
    bool empty;
};

bool empty(my_container c)
{
    return c.empty;
}

TEST_CASE("constraints::non_empty")
{
    constraints::non_empty p;
    REQUIRE(p(std::string("hi")));
    REQUIRE(!p(std::string()));

    REQUIRE(p(my_container{false}));
    REQUIRE(!p(my_container{true}));
}

TEST_CASE("constraints::non_default")
{
    constraints::non_default p;
    REQUIRE(p(5));
    REQUIRE(p(-1));
    REQUIRE(!p(int()));
}

TEST_CASE("constraints::non_invalid")
{
    constraints::non_invalid p;

    REQUIRE(p(&p));
    REQUIRE(!p(static_cast<void*>(nullptr)));

    REQUIRE(!p(false));
    REQUIRE(p(true));

    struct my_bool
    {
        bool b;

        explicit operator bool() const noexcept
        {
            return b;
        }
    };
    REQUIRE(p(my_bool{true}));
    REQUIRE(!p(my_bool{false}));
}
