
#include "dpso_utils/str.h"

#include "flow.h"
#include "utils.h"


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


void testStr(
    const char* callStr,
    const std::string& callResult,
    const char* expected,
    int lineNum)
{
    if (callResult != expected)
        test::failure(
            "line {}: {}: expected {}, got {}\n",
            lineNum,
            callStr,
            test::utils::toStr(expected),
            test::utils::toStr(callResult));
}

#define TEST_STR(CALL, EXPECTED) \
    testStr(#CALL, CALL, EXPECTED, __LINE__)


void testJustify()
{
    using dpso::str::leftJustify;
    using dpso::str::rightJustify;

    TEST_STR(leftJustify("x", 4), "x   ");
    TEST_STR(rightJustify("x", 4), "   x");

    TEST_STR(leftJustify("x", 4, '-'), "x---");
    TEST_STR(rightJustify("x", 4, '-'), "---x");

    TEST_STR(leftJustify("abcd", 2), "abcd");
    TEST_STR(rightJustify("abcd", 2), "abcd");
}


void testToStr()
{
    using dpso::str::toStr;

    TEST_STR(toStr(123), "123");
    TEST_STR(toStr(-123), "-123");

    TEST_STR(toStr(123, 2), "1111011");
    TEST_STR(toStr(-123, 2), "-1111011");

    TEST_STR(toStr(123, 8), "173");
    TEST_STR(toStr(-123, 8), "-173");

    TEST_STR(toStr(123, 10), "123");
    TEST_STR(toStr(-123, 10), "-123");

    TEST_STR(toStr(123, 16), "7b");
    TEST_STR(toStr(-123, 16), "-7b");

    TEST_STR(toStr(1.0), "1");
    TEST_STR(toStr(-1.0), "-1");
    TEST_STR(toStr(1.0101), "1.0101");
    TEST_STR(toStr(-1.0101), "-1.0101");
    TEST_STR(toStr(123.456), "123.456");
    TEST_STR(toStr(-123.456), "-123.456");
}


void testStr()
{
    testCmpSubStr();
    testJustify();
    testToStr();
}


}


REGISTER_TEST(testStr);
