// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/strong_typedef.hpp>

#include <catch.hpp>

#include <sstream>

using namespace type_safe;

TEST_CASE("strong_typedef")
{
    SECTION("equality_comparision")
    {
        struct type : strong_typedef<type, int>, strong_typedef_op::equality_comparision<type, bool>
        {
            using strong_typedef::strong_typedef;
        };

        type a(0);
        type b(1);

        REQUIRE(a == a);
        REQUIRE(!(a == b));

        REQUIRE(a != b);
        REQUIRE(!(a != a));
    }
    SECTION("relational_comparision")
    {
        struct type : strong_typedef<type, int>,
                      strong_typedef_op::relational_comparision<type, bool>
        {
            using strong_typedef::strong_typedef;
        };

        type a(0);
        type b(1);

        REQUIRE(a < b);
        REQUIRE(!(b < a));
        REQUIRE(a <= b);
        REQUIRE(a <= a);
        REQUIRE(b > a);
        REQUIRE(!(a > b));
        REQUIRE(b >= a);
        REQUIRE(b >= b);
    }
    SECTION("addition")
    {
        struct type : strong_typedef<type, int>,
                      strong_typedef_op::addition<type>,
                      strong_typedef_op::mixed_addition<type, int>
        {
            using strong_typedef::strong_typedef;
        };

        type a(0);
        a += type(1);
        a = a + type(1);
        a = type(1) + a;
        REQUIRE(static_cast<int>(a) == 3);

        type b(0);
        b += 1;
        b = b + 1;
        b = 1 + b;
        REQUIRE(static_cast<int>(b) == 3);
    }
    SECTION("subtraction")
    {
        struct type : strong_typedef<type, int>,
                      strong_typedef_op::subtraction<type>,
                      strong_typedef_op::mixed_subtraction<type, int>
        {
            using strong_typedef::strong_typedef;
        };

        type a(0);
        a -= type(1);    // -1
        a = a - type(1); // -2
        a = type(1) - a; // 3
        REQUIRE(static_cast<int>(a) == 3);

        type b(0);
        b -= 1;
        b = b - 1;
        b = 1 - b;
        REQUIRE(static_cast<int>(b) == 3);
    }
    SECTION("multiplication")
    {
        struct type : strong_typedef<type, int>,
                      strong_typedef_op::multiplication<type>,
                      strong_typedef_op::mixed_multiplication<type, int>
        {
            using strong_typedef::strong_typedef;
        };

        type a(1);
        a *= type(2);
        a = a * type(2);
        a = type(2) * a;
        REQUIRE(static_cast<int>(a) == 8);

        type b(1);
        b *= 2;
        b = b * 2;
        b = 2 * b;
        REQUIRE(static_cast<int>(b) == 8);
    }
    SECTION("division")
    {
        struct type : strong_typedef<type, int>,
                      strong_typedef_op::division<type>,
                      strong_typedef_op::mixed_division<type, int>
        {
            using strong_typedef::strong_typedef;
        };

        type a(8);
        a /= type(2);
        a = a / type(2);
        a = type(2) / a;
        REQUIRE(static_cast<int>(a) == 1);

        type b(8);
        b /= 2;
        b = b / 2;
        b = 2 / b;
        REQUIRE(static_cast<int>(b) == 1);
    }
    SECTION("modulo")
    {
        struct type : strong_typedef<type, int>,
                      strong_typedef_op::modulo<type>,
                      strong_typedef_op::mixed_modulo<type, int>
        {
            using strong_typedef::strong_typedef;
        };

        type a(11);
        a %= type(6);    // 5
        a = a % type(2); // 1
        a = type(2) % a; // 0
        REQUIRE(static_cast<int>(a) == 0);

        type b(11);
        b %= 6;
        b = b % 2;
        b = 2 % b;
        REQUIRE(static_cast<int>(b) == 0);
    }
    SECTION("increment")
    {
        struct type : strong_typedef<type, int>, strong_typedef_op::increment<type>
        {
            using strong_typedef::strong_typedef;
        };

        type a(0);
        REQUIRE(static_cast<int>(++a) == 1);
        REQUIRE(static_cast<int>(a++) == 1);
        REQUIRE(static_cast<int>(a) == 2);
    }
    SECTION("decrement")
    {
        struct type : strong_typedef<type, int>, strong_typedef_op::decrement<type>
        {
            using strong_typedef::strong_typedef;
        };

        type a(0);
        REQUIRE(static_cast<int>(--a) == -1);
        REQUIRE(static_cast<int>(a--) == -1);
        REQUIRE(static_cast<int>(a) == -2);
    }
    SECTION("unary")
    {
        struct type : strong_typedef<type, int>,
                      strong_typedef_op::unary_minus<type>,
                      strong_typedef_op::unary_plus<type>
        {
            using strong_typedef::strong_typedef;
        };

        type a(2);
        REQUIRE(static_cast<int>(+a) == 2);
        REQUIRE(static_cast<int>(-a) == -2);
    }
    SECTION("dereference")
    {
        struct test
        {
            int a;
        } t{0};

        struct type : strong_typedef<type, test*>, strong_typedef_op::dereference<type, test>
        {
            using strong_typedef::strong_typedef;
        };

        type a(&t);
        REQUIRE((*a).a == 0);
        REQUIRE(a->a == 0);
    }
    SECTION("array subscript")
    {
        int arr[] = {0, 1, 2};

        struct type : strong_typedef<type, int*>, strong_typedef_op::array_subscript<type, int>
        {
            using strong_typedef::strong_typedef;
        };

        type a(arr);
        REQUIRE(a[0] == 0);
        REQUIRE(a[1] == 1);
        REQUIRE(a[2] == 2);
    }
    SECTION("iterator")
    {
        int arr[] = {0, 1, 2};

        struct type : strong_typedef<type, int*>,
                      strong_typedef_op::random_access_iterator<type, int>
        {
            using strong_typedef::strong_typedef;
        };

        type a(arr);
        a += 1;
        REQUIRE(a == type(&arr[1]));

        a -= 1;
        REQUIRE(a == type(&arr[0]));

        a = a + 1;
        a = 1 + a;
        REQUIRE(a == type(&arr[2]));

        a = a - 1;
        REQUIRE(a == type(&arr[1]));

        REQUIRE(a - type(&arr[0]) == 1);
    }
    SECTION("i/o")
    {
        struct type : strong_typedef<type, int>,
                      strong_typedef_op::input_operator<type>,
                      strong_typedef_op::output_operator<type>
        {
            using strong_typedef::strong_typedef;
        };

        std::ostringstream out;
        std::istringstream in("1");

        type a(0);
        out << a;
        REQUIRE(out.str() == "0");

        in >> a;
        REQUIRE(static_cast<int>(a) == 1);
    }
}
