// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/bounded_type.hpp>

#include <catch.hpp>

using namespace type_safe;

TEST_CASE("constraints::less")
{
    constraints::less<int> p(42);
    REQUIRE(p.get_bound() == 42);
    REQUIRE(p(0));
    REQUIRE(p(40));
    REQUIRE(!p(42));
    REQUIRE(!p(50));
    REQUIRE(!p(100));
}

TEST_CASE("constraints::less_equal")
{
    constraints::less_equal<int> p(42);
    REQUIRE(p.get_bound() == 42);
    REQUIRE(p(0));
    REQUIRE(p(40));
    REQUIRE(p(42));
    REQUIRE(!p(50));
    REQUIRE(!p(100));
}

TEST_CASE("constraints::greater")
{
    constraints::greater<int> p(42);
    REQUIRE(p.get_bound() == 42);
    REQUIRE(!p(0));
    REQUIRE(!p(40));
    REQUIRE(!p(42));
    REQUIRE(p(50));
    REQUIRE(p(100));
}

TEST_CASE("constraints::greater_equal")
{
    constraints::greater_equal<int> p(42);
    REQUIRE(p.get_bound() == 42);
    REQUIRE(!p(0));
    REQUIRE(!p(40));
    REQUIRE(p(42));
    REQUIRE(p(50));
    REQUIRE(p(100));
}

TEST_CASE("constraints::bounded")
{
    SECTION("closed, closed")
    {
        constraints::bounded<int, true, true> p(0, 42);
        static_assert(std::is_same<decltype(p), constraints::closed_interval<int>>::value, "");
        REQUIRE(p.get_lower_bound() == 0);
        REQUIRE(p.get_upper_bound() == 42);

        REQUIRE(p(30));
        REQUIRE(p(41));
        REQUIRE(p(1));

        REQUIRE(p(0));
        REQUIRE(p(42));

        REQUIRE(!p(-5));
        REQUIRE(!p(100));
    }
    SECTION("open, closed")
    {
        constraints::bounded<int, false, true> p(0, 42);
        REQUIRE(p.get_lower_bound() == 0);
        REQUIRE(p.get_upper_bound() == 42);

        REQUIRE(p(30));
        REQUIRE(p(41));
        REQUIRE(p(1));

        REQUIRE(!p(0));
        REQUIRE(p(42));

        REQUIRE(!p(-5));
        REQUIRE(!p(100));
    }
    SECTION("closed, open")
    {
        constraints::bounded<int, true, false> p(0, 42);
        REQUIRE(p.get_lower_bound() == 0);
        REQUIRE(p.get_upper_bound() == 42);

        REQUIRE(p(30));
        REQUIRE(p(41));
        REQUIRE(p(1));

        REQUIRE(p(0));
        REQUIRE(!p(42));

        REQUIRE(!p(-5));
        REQUIRE(!p(100));
    }
    SECTION("open, open")
    {
        constraints::bounded<int, false, false> p(0, 42);
        static_assert(std::is_same<decltype(p), constraints::open_interval<int>>::value, "");
        REQUIRE(p.get_lower_bound() == 0);
        REQUIRE(p.get_upper_bound() == 42);

        REQUIRE(p(30));
        REQUIRE(p(41));
        REQUIRE(p(1));

        REQUIRE(!p(0));
        REQUIRE(!p(42));

        REQUIRE(!p(-5));
        REQUIRE(!p(100));
    }
}

TEST_CASE("clamping_verifier")
{
    SECTION("less_equal")
    {
        constraints::less_equal<int> p(42);

        int a = 0;
        clamping_verifier::verify(a, p);
        REQUIRE(a == 0);

        int b = 30;
        clamping_verifier::verify(b, p);
        REQUIRE(b == 30);

        int c = 42;
        clamping_verifier::verify(c, p);
        REQUIRE(c == 42);

        int d = 50;
        clamping_verifier::verify(d, p);
        REQUIRE(d == 42);
    }
    SECTION("greater_equal")
    {
        constraints::greater_equal<int> p(42);

        int a = 0;
        clamping_verifier::verify(a, p);
        REQUIRE(a == 42);

        int b = 30;
        clamping_verifier::verify(b, p);
        REQUIRE(b == 42);

        int c = 42;
        clamping_verifier::verify(c, p);
        REQUIRE(c == 42);

        int d = 50;
        clamping_verifier::verify(d, p);
        REQUIRE(d == 50);
    }
    SECTION("closed_interval")
    {
        constraints::closed_interval<int> p(0, 42);

        int a = 30;
        clamping_verifier::verify(a, p);
        REQUIRE(a == 30);

        int b = 10;
        clamping_verifier::verify(b, p);
        REQUIRE(b == 10);

        int c = 0;
        clamping_verifier::verify(c, p);
        REQUIRE(c == 0);

        int d = 42;
        clamping_verifier::verify(d, p);
        REQUIRE(d == 42);

        int e = 50;
        clamping_verifier::verify(e, p);
        REQUIRE(e == 42);

        int f = -20;
        clamping_verifier::verify(f, p);
        REQUIRE(f == 0);
    }
}
