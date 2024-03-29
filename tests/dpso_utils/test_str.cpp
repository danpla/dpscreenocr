
#include "dpso_utils/str.h"

#include "flow.h"


namespace {


enum class Order {
    less,
    equal,
    greater
};


Order getOrder(int cmpResult)
{
    if (cmpResult < 0)
        return Order::less;

    if (cmpResult == 0)
        return Order::equal;

    return Order::greater;
}


const char* toStr(Order order)
{
    switch (order) {
    case Order::less:
        return "<";
    case Order::equal:
        return "==";
    case Order::greater:
        return ">";
    }

    return "";
}


void testCmpSubStr()
{
    using namespace dpso::str;

    const struct Test {
        const char* str;
        const char* subStr;
        std::size_t subStrLen;
        unsigned cmpOptions;
        Order expectedOrder;
    } tests[] = {
        {"", "", 0, cmpNormal, Order::equal},
        {"", "", 1, cmpNormal, Order::equal},
        {"", "", 2, cmpNormal, Order::equal},

        {"Foo", "Foo", 0, cmpNormal, Order::greater},
        {"Foo", "Foo", 1, cmpNormal, Order::greater},
        {"Foo", "Foo", 2, cmpNormal, Order::greater},
        {"Foo", "Foo", 3, cmpNormal, Order::equal},
        {"Foo", "Foo", 4, cmpNormal, Order::equal},

        {"Foo", "FooBar", 2, cmpNormal, Order::greater},
        {"Foo", "FooBar", 3, cmpNormal, Order::equal},
        {"Foo", "FooBar", 4, cmpNormal, Order::less},

        {"FooBar", "Foo", 0, cmpNormal, Order::greater},
        {"FooBar", "Foo", 3, cmpNormal, Order::greater},
        {"FooBar", "Foo", 6, cmpNormal, Order::greater},
        {"FooBar", "Foo", 9, cmpNormal, Order::greater},

        {"Foo", "foo", 3, cmpNormal, Order::less},
        {"foo", "Foo", 3, cmpNormal, Order::greater},
        {"Foo", "foo", 3, cmpIgnoreCase, Order::equal},
    };

    for (const auto& test : tests) {
        const auto gotOrder = getOrder(cmpSubStr(
            test.str, test.subStr, test.subStrLen, test.cmpOptions));

        if (gotOrder == test.expectedOrder)
            continue;

        test::failure(
            "testCmpSubStr: cmpSubStr(\"{}\", \"{}\", {}, {}): "
            "expected {}, got {}\n",
            test.str,
            test.subStr,
            test.subStrLen,
            test.cmpOptions,
            toStr(test.expectedOrder),
            toStr(gotOrder));
    }
}


void testStr()
{
    testCmpSubStr();
}


}


REGISTER_TEST(testStr);
