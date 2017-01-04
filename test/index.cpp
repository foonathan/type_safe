// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/index.hpp>

#include <catch.hpp>

using namespace type_safe;

TEST_CASE("index_t")
{
    index_t idx;
    REQUIRE(idx == index_t(0u));

    SECTION("operator+=")
    {
        distance_t a(5);
        idx += a;
        REQUIRE(idx == index_t(5u));

        distance_t b(-3);
        idx += b;
        REQUIRE(idx == index_t(2u));
    }
    SECTION("operator-=")
    {
        idx = index_t(10u);

        distance_t a(5);
        idx -= a;
        REQUIRE(idx == index_t(5u));

        distance_t b(-3);
        idx -= b;
        REQUIRE(idx == index_t(8u));
    }
    SECTION("operator+")
    {
        auto c = idx + distance_t(5);
        REQUIRE(c == index_t(5u));

        auto d = distance_t(5) + idx;
        REQUIRE(d == index_t(5u));

        auto e = c + distance_t(-3);
        REQUIRE(e == index_t(2u));

        auto f = distance_t(-3) + d;
        REQUIRE(f == index_t(2u));
    }
    SECTION("next")
    {
        auto a = next(idx, distance_t(5));
        REQUIRE(a == index_t(5u));

        auto b = next(a, distance_t(-3));
        REQUIRE(b == index_t(2u));
    }
    SECTION("prev")
    {
        idx = index_t(10u);

        auto a = prev(idx, distance_t(5));
        REQUIRE(a == index_t(5u));

        auto b = prev(a, distance_t(-3));
        REQUIRE(b == index_t(8u));
    }
    SECTION("advance")
    {
        advance(idx, distance_t(5));
        REQUIRE(idx == index_t(5u));

        advance(idx, distance_t(-3));
        REQUIRE(idx == index_t(2u));
    }
    SECTION("operator-")
    {
        idx = index_t(10u);

        auto c = idx - distance_t(5);
        REQUIRE(c == index_t(5u));

        auto d = c - distance_t(-3);
        REQUIRE(d == index_t(8u));
    }
    SECTION("distance")
    {
        auto a = index_t(5u) - idx;
        REQUIRE(a == distance_t(5));
        REQUIRE(a == distance(idx, index_t(5u)));

        auto b = idx - index_t(5u);
        REQUIRE(b == distance_t(-5));
        REQUIRE(b == distance(index_t(5u), idx));
    }
    SECTION("at")
    {
        std::size_t array[] = {0, 1, 2, 3, 4, 5};

        for (index_t i; i != 5u; ++i)
            REQUIRE(at(array, i) == std::size_t(get(i)));
    }
}
