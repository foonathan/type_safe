// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/tagged_union.hpp>

#include <catch.hpp>

#include "debugger_type.hpp"

using namespace type_safe;

TEST_CASE("tagged_union")
{
    using union_t = tagged_union<int, float, debugger_type>;
    REQUIRE(union_t::invalid_type == union_t::type_id());

    union_t tunion;
    REQUIRE(!tunion.has_value());
    REQUIRE(tunion.type() == union_t::invalid_type);

    SECTION("emplace int")
    {
        tunion.emplace(union_type<int>{}, 5);
        REQUIRE(tunion.has_value());
        REQUIRE(tunion.type() == union_t::type_id(union_type<int>{}));
        REQUIRE(tunion.value(union_type<int>{}) == 5);

        union_t other;
        copy(other, tunion);
        REQUIRE(other.has_value());
        REQUIRE(other.type() == union_t::type_id(union_type<int>{}));
        REQUIRE(other.value(union_type<int>{}) == 5);

        SECTION("member")
        {
            tunion.destroy(union_type<int>{});
            REQUIRE(!tunion.has_value());
            REQUIRE(tunion.type() == union_t::invalid_type);
        }
        SECTION("non-member")
        {
            destroy(tunion);
            REQUIRE(!tunion.has_value());
            REQUIRE(tunion.type() == union_t::invalid_type);
        }
    }
    SECTION("emplace float")
    {
        tunion.emplace(union_type<float>{}, 3.0);
        REQUIRE(tunion.has_value());
        REQUIRE(tunion.type() == union_t::type_id(union_type<float>{}));
        REQUIRE(tunion.value(union_type<float>{}) == 3.0);

        union_t other;
        copy(other, tunion);
        REQUIRE(other.has_value());
        REQUIRE(other.type() == union_t::type_id(union_type<float>{}));
        REQUIRE(other.value(union_type<float>{}) == 3.0);

        SECTION("member")
        {
            tunion.destroy(union_type<float>{});
            REQUIRE(!tunion.has_value());
            REQUIRE(tunion.type() == union_t::invalid_type);
        }
        SECTION("non-member")
        {
            destroy(tunion);
            REQUIRE(!tunion.has_value());
            REQUIRE(tunion.type() == union_t::invalid_type);
        }
    }
    SECTION("emplace debugger_type")
    {
        tunion.emplace(union_type<debugger_type>{}, 42);
        REQUIRE(tunion.has_value());
        REQUIRE(tunion.type() == union_t::type_id(union_type<debugger_type>{}));

        auto& val = tunion.value(union_type<debugger_type>{});
        REQUIRE(val.id == 42);
        REQUIRE(val.ctor());

        SECTION("copy")
        {
            union_t other;
            copy(other, tunion);
            REQUIRE(other.has_value());
            REQUIRE(other.type() == union_t::type_id(union_type<debugger_type>{}));

            auto& otherval = other.value(union_type<debugger_type>{});
            REQUIRE(otherval.id == 42);
            REQUIRE(otherval.copy_ctor());
        }
        SECTION("move")
        {
            union_t other;
            move(other, std::move(tunion));
            REQUIRE(other.has_value());
            REQUIRE(other.type() == union_t::type_id(union_type<debugger_type>{}));

            auto& otherval = other.value(union_type<debugger_type>{});
            REQUIRE(otherval.id == 42);
            REQUIRE(otherval.move_ctor());
        }

        SECTION("member")
        {
            tunion.destroy(union_type<debugger_type>{});
            REQUIRE(!tunion.has_value());
            REQUIRE(tunion.type() == union_t::invalid_type);
        }
        SECTION("non-member")
        {
            destroy(tunion);
            REQUIRE(!tunion.has_value());
            REQUIRE(tunion.type() == union_t::invalid_type);
        }
    }
}
