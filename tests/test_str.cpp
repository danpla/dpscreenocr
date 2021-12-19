
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "dpso/str.h"

#include "flow.h"


enum class Order {
    less,
    equal,
    greater
};


static const char* orderToStr(Order order)
{
    if (order == Order::less)
        return "<";
    else if (order == Order::equal)
        return "==";
    else
        return ">";
}


static void checkCmpSubStr(
    const char* str,
    const char* subStr, std::size_t subStrLen,
    unsigned options,
    Order expected,
    int lineNum)
{
    const auto ret = dpso::str::cmpSubStr(
        str, subStr, subStrLen, options);

    Order got;
    if (ret < 0)
        got = Order::less;
    else if (ret == 0)
        got = Order::equal;
    else
        got = Order::greater;

    if (got == expected)
        return;

    std::fprintf(
        stderr,
        "line %i: equalSubStr("
        "\"%s\", \"%.*s\", "
        "%i, %u): "
        "got %s, expected %s\n",
        lineNum,
        str, static_cast<int>(subStrLen), subStr,
        static_cast<int>(subStrLen), options,
        orderToStr(got), orderToStr(expected));
    test::failure();
}


static void testCmpSubStr()
{
    using namespace dpso::str;

    #define CHECK(str, subStr, subStrLen, options, expected) \
        checkCmpSubStr( \
            str, subStr, subStrLen, options, expected, __LINE__) \

    for (int i = 0; i < 3; ++i) {
        CHECK("", "", i, cmpNormal, Order::equal);
        CHECK("Foo", "Foo", i, cmpNormal, Order::greater);
    }

    CHECK("Foo", "Foo", 3, cmpNormal, Order::equal);
    CHECK("Foo", "Foo", 4, cmpNormal, Order::equal);
    CHECK("Foo", "FooBar", 3, cmpNormal, Order::equal);
    CHECK("Foo", "FooBar", 4, cmpNormal, Order::less);

    for (int i = 0; i < 10; ++i)
        CHECK("FooBar", "Foo", i, cmpNormal, Order::greater);

    CHECK("Foo", "foo", 3, cmpNormal, Order::less);
    CHECK("Foo", "foo", 3, cmpIgnoreCase, Order::equal);

    #undef CHECK
}


static void testStr()
{
    testCmpSubStr();
}


REGISTER_TEST(testStr);
