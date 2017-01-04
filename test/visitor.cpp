// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/visitor.hpp>

#include <catch.hpp>

using namespace type_safe;

TEST_CASE("visit optional")
{
    struct visitor
    {
        using incomplete_visitor = void;

        int value;

        void operator()(nullopt_t) const
        {
            REQUIRE(value == -1);
        }

        void operator()(int i) const
        {
            REQUIRE(value == i);
        }

        void operator()(int, nullopt_t) const
        {
            REQUIRE(value == -1);
        }

        void operator()(int, int b) const
        {
            REQUIRE(value == b);
        }
    };

    optional<int> a;
    visit(visitor{-1}, a);

    a = 42;
    visit(visitor{42}, a);

    optional<int> b;
    visit(visitor{-1}, a, b);

    b = 32;
    visit(visitor{32}, a, b);
}

TEST_CASE("visit variant")
{
    struct visitor
    {
        using incomplete_visitor = void;

        int value;

        void operator()(nullvar_t) const
        {
            REQUIRE(value == -1);
        }

        void operator()(int i) const
        {
            REQUIRE(value == i);
        }

        void operator()(float f) const
        {
            REQUIRE(f == 3.14f);
        }

        void operator()(int, nullvar_t) const
        {
            REQUIRE(value == -1);
        }

        void operator()(int, int b) const
        {
            REQUIRE(value == b);
        }

        void operator()(float a, int b) const
        {
            REQUIRE(value == b);
            REQUIRE(a == 3.14f);
        }
    };

    variant<nullvar_t, int, float> a;
    visit(visitor{-1}, a);

    a = 42;
    visit(visitor{42}, a);

    variant<nullvar_t, int> b;
    visit(visitor{-1}, a, b);

    b = 32;
    visit(visitor{32}, a, b);

    a = 3.14f;
    visit(visitor{}, a);
    visit(visitor{32}, a, b);
}
