// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/optional_ref.hpp>

#include <catch.hpp>

#include "debugger_type.hpp"

using namespace type_safe;

TEST_CASE("optional_ref")
{
    // only test stuff special for optional_ref

    int value = 0;
    SECTION("constructor")
    {
        optional_ref<int> a(nullptr);
        REQUIRE_FALSE(a.has_value());

        optional_ref<int> b(value);
        REQUIRE(b.has_value());
        REQUIRE(&b.value() == &value);

        static_assert(!std::is_constructible<optional_ref<int>, int&&>::value, "");
    }
    SECTION("assignment")
    {
        optional_ref<int> a;
        a = nullptr;
        REQUIRE_FALSE(a.has_value());

        optional_ref<int> b;
        b = value;
        REQUIRE(b.has_value());
        REQUIRE(&b.value() == &value);

        static_assert(!std::is_assignable<optional_ref<int>, int&&>::value, "");
    }
    SECTION("value_or")
    {
        int v1 = 0, v2 = 0;

        optional_ref<int> a;
        a.value_or(v2) = 1;
        REQUIRE(v2 == 1);
        REQUIRE(v1 == 0);
        v2 = 0;

        int res1 = a.value_or(3);
        REQUIRE(res1 == 3);

        optional_ref<int> b(v1);
        b.value_or(v2) = 1;
        REQUIRE(v1 == 1);
        REQUIRE(v2 == 0);

        int res2 = b.value_or(3);
        REQUIRE(res2 == v1);
    }
    SECTION("ref")
    {
        optional_ref<int> a = ref(static_cast<int*>(nullptr));
        REQUIRE_FALSE(a.has_value());

        optional_ref<int> b = ref(&value);
        REQUIRE(b.has_value());
        REQUIRE(&b.value() == &value);
    }
    SECTION("cref")
    {
        optional_ref<const int> a = cref(static_cast<const int*>(nullptr));
        REQUIRE_FALSE(a.has_value());

        optional_ref<const int> b = cref(&value);
        REQUIRE(b.has_value());
        REQUIRE(&b.value() == &value);
    }
    SECTION("copy")
    {
        debugger_type dbg(0);

        optional_ref<debugger_type> a;
        optional<debugger_type>     a_res = copy(a);
        REQUIRE_FALSE(a_res.has_value());

        optional_ref<debugger_type> b(dbg);
        optional<debugger_type>     b_res = copy(b);
        REQUIRE(b_res.has_value());
        REQUIRE(b_res.value().id == 0);
    }
}

TEST_CASE("optional_xvalue_ref")
{
    int value = 0;

    SECTION("constructor")
    {
        optional_xvalue_ref<int> a(nullptr);
        REQUIRE_FALSE(a.has_value());

        optional_xvalue_ref<int> b(value);
        REQUIRE(b.has_value());
        REQUIRE(b.value() == value);

        static_assert(!std::is_constructible<optional_xvalue_ref<int>, int&&>::value, "");
    }
    SECTION("assignment")
    {
        optional_xvalue_ref<int> a;
        a = nullptr;
        REQUIRE_FALSE(a.has_value());

        optional_xvalue_ref<int> b;
        b = value;
        REQUIRE(b.has_value());
        REQUIRE(b.value() == value);

        static_assert(!std::is_assignable<optional_xvalue_ref<int>, int&&>::value, "");
    }
    SECTION("value_or")
    {
        int v1 = 1, v2 = 0;

        optional_xvalue_ref<int> a;
        REQUIRE(a.value_or(v2) == v2);
        REQUIRE(a.value_or(3) == 3);

        optional_xvalue_ref<int> b(v1);
        REQUIRE(b.value_or(v2) == v1);
        REQUIRE(b.value_or(3) == v1);
    }
    SECTION("xref")
    {
        optional_xvalue_ref<int> a = xref(static_cast<int*>(nullptr));
        REQUIRE_FALSE(a.has_value());

        optional_xvalue_ref<int> b = xref(&value);
        REQUIRE(b.has_value());
        REQUIRE(b.value() == value);
    }
    SECTION("move")
    {
        debugger_type dbg(0);

        optional_xvalue_ref<debugger_type> a;
        optional<debugger_type>            a_res = move(a);
        REQUIRE_FALSE(a_res.has_value());

        optional_xvalue_ref<debugger_type> b(dbg);
        optional<debugger_type>            b_res = move(b);
        REQUIRE(b_res.has_value());
        REQUIRE(b_res.value().id == 0);
#ifndef _MSC_VER
        REQUIRE(b_res.value().move_ctor());
#endif
    }
}
