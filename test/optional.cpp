// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/optional.hpp>

#include <catch.hpp>

using namespace type_safe;

struct debugger_type
{
    int  id;
    bool from_move_ctor    = false;
    bool from_copy_ctor    = false;
    bool was_move_assigned = false;
    bool was_copy_assigned = false;
    bool swapped           = false;

    debugger_type(int id) : id(id)
    {
    }

    debugger_type(debugger_type&& other) : debugger_type(other.id)
    {
        from_move_ctor = true;
    }

    debugger_type(const debugger_type& other) : debugger_type(other.id)
    {
        from_copy_ctor = true;
    }

    ~debugger_type() = default;

    debugger_type& operator=(debugger_type&& other)
    {
        id                = other.id;
        was_move_assigned = true;
        return *this;
    }

    debugger_type& operator=(const debugger_type& other)
    {
        id                = other.id;
        was_copy_assigned = true;
        return *this;
    }

    friend void swap(debugger_type& a, debugger_type& b) noexcept
    {
        std::swap(a.id, b.id);
        a.swapped = b.swapped = true;
    }

    bool ctor() const
    {
        return !from_copy_ctor && !from_move_ctor;
    }

    bool move_ctor() const
    {
        return from_move_ctor && !from_copy_ctor;
    }

    bool copy_ctor() const
    {
        return from_copy_ctor && !from_move_ctor;
    }

    bool not_assigned() const
    {
        return !was_copy_assigned && !was_move_assigned;
    }

    bool move_assigned() const
    {
        return was_move_assigned && !was_copy_assigned;
    }

    bool copy_assigned() const
    {
        return was_copy_assigned && !was_move_assigned;
    }
};

TEST_CASE("optional")
{
    /*    SECTION("constructor - empty")
    {
        optional<int> a;
        REQUIRE_FALSE(a.has_value());

        optional<int> b(nullopt);
        REQUIRE_FALSE(b.has_value());
    }
    SECTION("constructor - value")
    {
        optional<debugger_type> a(debugger_type(3));
        REQUIRE(a.has_value());
        REQUIRE(a.value().id == 3);
        REQUIRE(a.value().move_ctor());

        debugger_type           dbg(2);
        optional<debugger_type> b(dbg);
        REQUIRE(b.has_value());
        REQUIRE(b.value().id == 2);
        REQUIRE(b.value().copy_ctor());

        optional<debugger_type> c(std::move(dbg));
        REQUIRE(c.has_value());
        REQUIRE(c.value().id == 2);
        REQUIRE(c.value().move_ctor());

        optional<debugger_type> d(0);
        REQUIRE(d.has_value());
        REQUIRE(d.value().id == 0);
        REQUIRE(d.value().ctor());
    }
    SECTION("constructor - move/copy")
    {
        optional<debugger_type> org_empty;
        optional<debugger_type> org_value(debugger_type(0));

        optional<debugger_type> a(org_empty);
        REQUIRE_FALSE(a.has_value());

        optional<debugger_type> b(std::move(org_empty));
        REQUIRE_FALSE(b.has_value());

        optional<debugger_type> c(org_value);
        REQUIRE(c.has_value());
        REQUIRE(c.value().id == 0);
        REQUIRE(c.value().copy_ctor());

        optional<debugger_type> d(std::move(org_value));
        REQUIRE(d.has_value());
        REQUIRE(d.value().id == 0);
        REQUIRE(d.value().move_ctor());
    }
    SECTION("assignment - nullopt_t")
    {
        optional<debugger_type> a;
        a = nullopt;
        REQUIRE_FALSE(a.has_value());

        optional<debugger_type> b(4);
        b = nullopt;
        REQUIRE_FALSE(b.has_value());
    }
    SECTION("assignment - value")
    {
        optional<debugger_type> a;
        a = debugger_type(0);
        REQUIRE(a.has_value());
        REQUIRE(a.value().id == 0);
        REQUIRE(a.value().move_ctor());
        REQUIRE(a.value().not_assigned());

        debugger_type           dbg(0);
        optional<debugger_type> b;
        b = dbg;
        REQUIRE(b.has_value());
        REQUIRE(b.value().id == 0);
        REQUIRE(b.value().copy_ctor());
        REQUIRE(b.value().not_assigned());

        optional<debugger_type> c;
        c = 0;
        REQUIRE(c.has_value());
        REQUIRE(c.value().id == 0);
        REQUIRE(c.value().ctor());
        REQUIRE(c.value().not_assigned());

        optional<debugger_type> d(0);
        d = debugger_type(1);
        REQUIRE(d.has_value());
        REQUIRE(d.value().id == 1);
        REQUIRE(d.value().ctor());
        REQUIRE(d.value().move_assigned());

        dbg.id = 1;
        optional<debugger_type> e(0);
        e = dbg;
        REQUIRE(e.has_value());
        REQUIRE(e.value().id == 1);
        REQUIRE(e.value().ctor());
        REQUIRE(e.value().copy_assigned());

        optional<debugger_type> f(0);
        f = 1; // assignment would use implicit conversion, so it destroys & recreates
        REQUIRE(f.has_value());
        REQUIRE(f.value().id == 1);
        REQUIRE(f.value().ctor());
        REQUIRE(f.value().not_assigned());
    }
    SECTION("assignment - move/copy")
    {
        optional<debugger_type> new_empty;
        optional<debugger_type> new_value(debugger_type(1));

        optional<debugger_type> a;
        a = new_empty;
        REQUIRE_FALSE(a.has_value());

        optional<debugger_type> b;
        b = new_value;
        REQUIRE(b.has_value());
        REQUIRE(b.value().id == 1);
        REQUIRE(b.value().copy_ctor());
        REQUIRE(b.value().not_assigned());

        optional<debugger_type> c;
        c = std::move(new_value);
        REQUIRE(c.has_value());
        REQUIRE(c.value().id == 1);
        REQUIRE(c.value().move_ctor());
        REQUIRE(c.value().not_assigned());

        optional<debugger_type> d(0);
        d = new_empty;
        REQUIRE_FALSE(d.has_value());

        optional<debugger_type> e(0);
        e = new_value;
        REQUIRE(e.has_value());
        REQUIRE(e.value().id == 1);
        REQUIRE(e.value().ctor());
        REQUIRE(e.value().copy_assigned());

        optional<debugger_type> f(0);
        f = std::move(new_value);
        REQUIRE(f.has_value());
        REQUIRE(f.value().id == 1);
        REQUIRE(f.value().ctor());
        REQUIRE(f.value().move_assigned());
    }
    SECTION("swap")
    {
        optional<debugger_type> empty1, empty2;
        optional<debugger_type> a(0);
        optional<debugger_type> b(1);

        SECTION("empty, empty")
        {
            swap(empty1, empty2);
            REQUIRE_FALSE(empty1.has_value());
            REQUIRE_FALSE(empty2.has_value());
        }
        SECTION("value, value")
        {
            swap(a, b);
            REQUIRE(a.has_value());
            REQUIRE(a.value().id == 1);
            REQUIRE(a.value().swapped);
            REQUIRE(b.has_value());
            REQUIRE(b.value().id == 0);
            REQUIRE(b.value().swapped);
        }
        SECTION("empty, value")
        {
            swap(empty1, a);
            REQUIRE_FALSE(a.has_value());
            REQUIRE(empty1.has_value());
            REQUIRE(empty1.value().id == 0);
            REQUIRE(empty1.value().move_ctor());
            REQUIRE(empty1.value().not_assigned());
            REQUIRE_FALSE(empty1.value().swapped);
        }
        SECTION("value, empty")
        {
            swap(a, empty1);
            REQUIRE_FALSE(a.has_value());
            REQUIRE(empty1.has_value());
            REQUIRE(empty1.value().id == 0);
            REQUIRE(empty1.value().move_ctor());
            REQUIRE(empty1.value().not_assigned());
            REQUIRE_FALSE(empty1.value().swapped);
        }
    }
    SECTION("reset")
    {
        optional<debugger_type> a;
        a.reset();
        REQUIRE_FALSE(a.has_value());

        optional<debugger_type> b(0);
        b.reset();
        REQUIRE_FALSE(b.has_value());
    }
    SECTION("emplace")
    {
        debugger_type dbg(1);

        optional<debugger_type> a;
        a.emplace(1);
        REQUIRE(a.has_value());
        REQUIRE(a.value().id == 1);
        REQUIRE(a.value().ctor());
        REQUIRE(a.value().not_assigned());

        optional<debugger_type> b;
        b.emplace(dbg);
        REQUIRE(b.has_value());
        REQUIRE(b.value().id == 1);
        REQUIRE(b.value().copy_ctor());
        REQUIRE(b.value().not_assigned());

        optional<debugger_type> c;
        c.emplace(std::move(dbg));
        REQUIRE(c.has_value());
        REQUIRE(c.value().id == 1);
        REQUIRE(c.value().move_ctor());
        REQUIRE(c.value().not_assigned());

        optional<debugger_type> d(0);
        d.emplace(1);
        REQUIRE(d.has_value());
        REQUIRE(d.value().id == 1);
        REQUIRE(d.value().ctor());
        REQUIRE(d.value().not_assigned());

        optional<debugger_type> e(0);
        e.emplace(dbg);
        REQUIRE(e.has_value());
        REQUIRE(e.value().id == 1);
        REQUIRE(e.value().ctor());
        REQUIRE(e.value().copy_assigned());

        optional<debugger_type> f(0);
        f.emplace(std::move(dbg));
        REQUIRE(f.has_value());
        REQUIRE(f.value().id == 1);
        REQUIRE(f.value().ctor());
        REQUIRE(f.value().move_assigned());
    }
    SECTION("operator bool")
    {
        optional<int> a;
        REQUIRE_FALSE(static_cast<bool>(a));

        optional<int> b(0);
        REQUIRE(static_cast<bool>(b));
    }
    SECTION("value")
    {
        // only test the return types
        optional<debugger_type> a(0);
        static_assert(std::is_same<decltype(a.value()), debugger_type&>::value, "");
        static_assert(std::is_same<decltype(std::move(a).value()), debugger_type&&>::value, "");

        const optional<debugger_type> b(0);
        static_assert(std::is_same<decltype(b.value()), const debugger_type&>::value, "");
        static_assert(std::is_same<decltype(std::move(b).value()), const debugger_type&&>::value,
                      "");
    }
    SECTION("value_or")
    {
        optional<debugger_type> a;
        auto                    a_res = a.value_or(1);
        REQUIRE(a_res.id == 1);

        optional<debugger_type> b(0);
        auto                    b_res = b.value_or(1);
        REQUIRE(b_res.id == 0);
    }
    SECTION("unwrap")
    {
        optional<int> a;
        optional<int> a_res = a.unwrap();
        REQUIRE_FALSE(a_res.has_value());

        optional<optional<int>> b;
        optional<int>           b_res = b.unwrap();
        REQUIRE_FALSE(b_res.has_value());

        optional<int> c(0);
        optional<int> c_res = c.unwrap();
        REQUIRE(c_res.has_value());
        REQUIRE(c_res.value() == 0);

        optional<optional<int>> d{optional<int>(nullopt)};
        optional<int>           d_res = d.unwrap();
        REQUIRE_FALSE(d_res.has_value());

        optional<optional<int>> e{optional<int>(0)};
        optional<int>           e_res = e.unwrap();
        REQUIRE(e_res.has_value());
        REQUIRE(e_res.value() == 0);
    }
    SECTION("map")
    {
        auto func = [](int i) { return "abc"[i]; };

        optional<int>  a;
        optional<char> a_res = a.map(func);
        REQUIRE_FALSE(a_res.has_value());

        optional<int>  b(0);
        optional<char> b_res = b.map(func);
        REQUIRE(b_res.has_value());
        REQUIRE(b_res.value() == 'a');
    }
    SECTION("bind")
    {
        auto func1 = [](int i) { return "abc"[i]; };

        optional<int>  a;
        optional<char> a_res = a.bind(func1);
        REQUIRE_FALSE(a_res.has_value());

        optional<int>  b(0);
        optional<char> b_res = b.bind(func1);
        REQUIRE(b_res.has_value());
        REQUIRE(b_res.value() == 'a');

        auto func2 = [&](int i) { return i == 0 ? optional<char>(nullopt) : func1(i - 1); };

        optional<int>  c;
        optional<char> c_res = c.bind(func2);
        REQUIRE_FALSE(c_res.has_value());

        optional<int>  d(0);
        optional<char> d_res = d.bind(func2);
        REQUIRE_FALSE(d_res.has_value());

        optional<int>  e(1);
        optional<char> e_res = e.bind(func2);
        REQUIRE(e_res.has_value());
        REQUIRE(e_res.value() == 'a');
    }
    SECTION("transform")
    {
        auto func = [](int i) { return "abc"[i]; };

        optional<int> a;
        char          a_res = a.transform(char('\0'), func);
        REQUIRE(a_res == '\0');

        optional<int> b(0);
        char          b_res = b.transform(char('\0'), func);
        REQUIRE(b_res == 'a');
    }
    SECTION("then")
    {
        auto func1 = [](optional<int> i) { return i.value_or(-1); };

        optional<int> a;
        optional<int> a_res = a.then(func1);
        REQUIRE(a_res.has_value());
        REQUIRE(a_res.value() == -1);

        optional<int> b(0);
        optional<int> b_res = b.then(func1);
        REQUIRE(b_res.has_value());
        REQUIRE(b_res.value() == 0);

        auto func2 = [](optional<int> i) { return !i ? optional<char>(nullopt) : 'A' + i.value(); };

        optional<int>  c;
        optional<char> c_res = c.then(func2);
        REQUIRE_FALSE(c_res.has_value());

        optional<int>  d(0);
        optional<char> d_res = d.then(func2);
        REQUIRE(d_res.has_value());
        REQUIRE(d_res.value() == 'A');
    }
    SECTION("with")
    {
        optional<int> a;
        with(a, [](int) { REQUIRE(false); });

        a = 0;
        with(a, [](int& i) {
            REQUIRE(i == 0);
            i = 1;
        });
        REQUIRE(a.has_value());
        REQUIRE(a.value() == 1);
    }
    SECTION("visit")
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

            void operator()(int a, nullopt_t) const
            {
                REQUIRE(value == -1);
            }

            void operator()(int a, int b) const
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
    }*/
    SECTION("apply")
    {
        auto called = false;
        auto func1  = [&](int a, int b) {
            REQUIRE(called);
            REQUIRE(a == 0);
            REQUIRE(b == 1);
            return 2;
        };

        optional<int> a, b;

        optional<int> res = apply<optional<int>>(func1, a, b);
        REQUIRE(!res.has_value());

        a   = 0;
        res = apply<optional<int>>(func1, a, b);
        REQUIRE(!res.has_value());

        b      = 1;
        called = true;
        res    = apply<optional<int>>(func1, a, b);
        REQUIRE(res.has_value());
        REQUIRE(res.value() == 2);
    }
    SECTION("comparision")
    {
        optional<int> a;
        optional<int> b(1);
        optional<int> c(2);

        // ==
        REQUIRE(b == b);
        REQUIRE(!(b == c));
        REQUIRE(!(b == a));

        REQUIRE(a == nullopt);
        REQUIRE(nullopt == a);
        REQUIRE(!(b == nullopt));
        REQUIRE(!(nullopt == b));

        REQUIRE(b == 1);
        REQUIRE(!(a == 1));
        REQUIRE(!(1 == a));
        REQUIRE(!(c == 1));
        REQUIRE(!(1 == c));

        // !=
        REQUIRE(a != b);
        REQUIRE(b != c);
        REQUIRE(!(a != a));

        REQUIRE(b != nullopt);
        REQUIRE(nullopt != b);
        REQUIRE(!(a != nullopt));
        REQUIRE(!(nullopt != a));

        REQUIRE(b != 2);
        REQUIRE(2 != b);
        REQUIRE(a != 2);
        REQUIRE(2 != a);
        REQUIRE(!(c != 2));
        REQUIRE(!(2 != c));

        // <
        REQUIRE(a < b);
        REQUIRE(b < c);
        REQUIRE(!(c < b));
        REQUIRE(!(b < a));

        REQUIRE(!(a < nullopt));
        REQUIRE(!(nullopt < a));
        REQUIRE(!(b < nullopt));
        REQUIRE(nullopt < b);

        REQUIRE(a < 2);
        REQUIRE(!(2 < a));
        REQUIRE(!(c < 2));
        REQUIRE(!(2 < c));

        // <=
        REQUIRE(a <= b);
        REQUIRE(b <= c);
        REQUIRE(b <= b);
        REQUIRE(!(c <= b));

        REQUIRE(a <= nullopt);
        REQUIRE(nullopt <= a);
        REQUIRE(!(b <= nullopt));
        REQUIRE(nullopt <= b);

        REQUIRE(a <= 2);
        REQUIRE(!(2 <= a));
        REQUIRE(b <= 2);
        REQUIRE(!(2 <= b));
        REQUIRE(c <= 2);
        REQUIRE(2 <= c);

        // >
        REQUIRE(c > b);
        REQUIRE(b > a);
        REQUIRE(!(a > b));

        REQUIRE(b > nullopt);
        REQUIRE(!(nullopt > b));
        REQUIRE(!(a > nullopt));
        REQUIRE(!(nullopt > b));

        REQUIRE(c > 1);
        REQUIRE(!(1 > c));
        REQUIRE(!(b > 1));
        REQUIRE(!(1 > b));
        REQUIRE(!(a > 1));
        REQUIRE(1 > a);

        // >=
        REQUIRE(c >= b);
        REQUIRE(b >= a);
        REQUIRE(a >= a);
        REQUIRE(!(a >= b));

        REQUIRE(a >= nullopt);
        REQUIRE(nullopt >= a);
        REQUIRE(b >= nullopt);
        REQUIRE(!(nullopt >= b));

        REQUIRE(b >= 1);
        REQUIRE(1 >= b);
        REQUIRE(c >= 1);
        REQUIRE(!(1 >= c));
        REQUIRE(!(a >= 1));
        REQUIRE(1 >= a);
    }
    SECTION("make_optional")
    {
        optional<int> a = make_optional(5);
        REQUIRE(a.has_value());
        REQUIRE(a.value() == 5);

        optional<std::string> b = make_optional<std::string>(1, 'a');
        REQUIRE(b.has_value());
        REQUIRE(b.value() == "a");
    }
}

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

        optional_ref<int> b(v1);
        b.value_or(v2) = 1;
        REQUIRE(v1 == 1);
        REQUIRE(v2 == 0);
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
        REQUIRE(b_res.value().copy_ctor());
    }
    SECTION("move")
    {
        debugger_type dbg(0);

        optional_ref<debugger_type> a;
        optional<debugger_type>     a_res = move(a);
        REQUIRE_FALSE(a_res.has_value());

        optional_ref<debugger_type> b(dbg);
        optional<debugger_type>     b_res = move(b);
        REQUIRE(b_res.has_value());
        REQUIRE(b_res.value().id == 0);
        REQUIRE(b_res.value().move_ctor());
    }
}
