// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/reference.hpp>

#include <catch.hpp>

#include "debugger_type.hpp"

using namespace type_safe;

template <typename T>
void check_object_ref(const object_ref<T>& ref, const debugger_type& value)
{
    REQUIRE(value.ctor());
    REQUIRE(&ref.get() == &value);
    REQUIRE(&*ref == &value);
    REQUIRE(ref->id == value.id);
    REQUIRE(ref == value);
    REQUIRE(value == ref);
}

void check_object_ref(const xvalue_ref<debugger_type> ref, const debugger_type& value)
{
    REQUIRE(value.ctor());
    REQUIRE(ref->id == value.id);
    REQUIRE(ref == value);
    REQUIRE(value == ref);
}

TEST_CASE("object_ref")
{
    debugger_type       value(42);
    const debugger_type cvalue(128);

    // value ctor
    object_ref<debugger_type> a(value);
    check_object_ref(a, value);

    object_ref<const debugger_type> b(cvalue);
    check_object_ref(b, cvalue);

    object_ref<const debugger_type> c(value);
    check_object_ref(c, value);

    // ref ctor
    object_ref<debugger_type> d(a);
    check_object_ref(d, value);

    object_ref<const debugger_type> e(b);
    check_object_ref(e, cvalue);

    object_ref<const debugger_type> f(a);
    check_object_ref(f, value);

    // comparison
    REQUIRE(a == d);
    REQUIRE(b == e);
    REQUIRE(c == f);
    REQUIRE(a == c);
    REQUIRE(a != b);
    REQUIRE(a != e);

    REQUIRE(a != cvalue);
    REQUIRE(cvalue != a);
    REQUIRE(e != value);
    REQUIRE(value != e);

    // with
    with(a,
         [&](debugger_type& a, int i) {
             REQUIRE(i == 42);
             REQUIRE(a.id == value.id);
         },
         42);

    // ref/cref
    auto g = ref(value);
    static_assert(std::is_same<decltype(g), object_ref<debugger_type>>::value, "");
    check_object_ref(g, value);

    auto h = ref(cvalue);
    static_assert(std::is_same<decltype(h), object_ref<const debugger_type>>::value, "");
    check_object_ref(h, cvalue);

    auto i = cref(value);
    static_assert(std::is_same<decltype(i), object_ref<const debugger_type>>::value, "");
    check_object_ref(i, value);

    // copy/move
    auto value2 = copy(a);
    REQUIRE(value2.id == value.id);
    REQUIRE(value2.copy_ctor());

    xvalue_ref<debugger_type> j(value);
    check_object_ref(j, value);

    auto value3 = move(j);
    REQUIRE(value3.id == value.id);
    REQUIRE(value3.move_ctor());
}

TEST_CASE("array_ref")
{
    int array[3]  = {1, 2, 3};
    int array2[1] = {1};

    array_ref<int> ref(array);
    REQUIRE(ref.data() == array);
    REQUIRE((ref.size() == 3u));

    REQUIRE(ref[0u] == 1);
    REQUIRE(ref[1u] == 2);
    REQUIRE(ref[2u] == 3);

    ref[0u] = 100;
    REQUIRE(array[0] == 100);

    SECTION("ctor")
    {
        array_ref<int> a(array, array + 3);
        REQUIRE(a.data() == array);
        REQUIRE((a.size() == 3u));

        array_ref<int> b(array, 3u);
        REQUIRE(b.data() == array);
        REQUIRE((b.size() == 3u));

        array_ref<int> c(array);
        REQUIRE(c.data() == array);
        REQUIRE((c.size() == 3u));
    }
    SECTION("operator=")
    {
        ref = array2;
        REQUIRE(ref.data() == array2);
        REQUIRE((ref.size() == 1u));
    }
    SECTION("assign range")
    {
        ref.assign(array2, array2 + 1);
        REQUIRE(ref.data() == array2);
        REQUIRE((ref.size() == 1u));
    }
    SECTION("assign size")
    {
        ref.assign(array2, 1u);
        REQUIRE(ref.data() == array2);
        REQUIRE((ref.size() == 1u));
    }
    SECTION("assign array")
    {
        ref.assign(array2);
        REQUIRE(ref.data() == array2);
        REQUIRE((ref.size() == 1u));
    }
    SECTION("begin()/end()")
    {
        REQUIRE(ref.begin() == array);
        REQUIRE(ref.end() == array + 3);
    }
}
