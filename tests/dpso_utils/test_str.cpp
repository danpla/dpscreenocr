#include <cstdint>

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


void testCmpIgnoreCase()
{
    using namespace dpso::str;

    const struct {
        std::string_view a;
        std::string_view b;
        Order expectedOrder;
    } tests[]{
        {"", "", Order::equal},
        {"Foo", "FOO", Order::equal},
        {"Foo", "FOOBar", Order::less},
        {"FOOBar", "Foo", Order::greater},
    };

    for (const auto& test : tests) {
        const auto gotOrder = getOrder(cmpIgnoreCase(test.a, test.b));

        if (gotOrder == test.expectedOrder)
            continue;

        test::failure(
            "testCmpIgnoreCase: cmpIgnoreCase({}, {}): "
            "expected {}, got {}",
            test::utils::toStr(test.a),
            test::utils::toStr(test.b),
            test.expectedOrder,
            gotOrder);
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
            "line {}: {}: expected {}, got {}",
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

    TEST_STR(toStr('c'), "c");

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

    TEST_STR(
        toStr(std::int64_t{INT64_MIN}, 2),
        ("-1" + std::string(63, '0')).c_str());
}


void testFormat()
{
    const struct {
        const char* str;
        std::initializer_list<std::string_view> args;
        const char* expected;
    } tests[]{
        // Normal
        {"1: {}, 2: {}", {"a", "b"}, "1: a, 2: b"},
        // Nonexistent arg
        {"1: {}, 2: {}, 3: {}", {"a", "b"}, "1: a, 2: b, 3: {}"},
        // String within {}
        {"1: {x}, 2: {}", {"a", "b"}, "1: {x}, 2: a"},
        // Brace substitution
        {"1: {{}} {{ }}", {"a"}, "1: {} { }"},
        // Stray }
        {"1: {}, 2: {{}, 3: {}", {"a", "b"}, "1: a, 2: {}, 3: {}"},
        // { within name
        {"1: {}, 2: { {}, 3: {}", {"a", "b"}, "1: a, 2: { {}, 3: {}"},
        // No closing }
        {"1: {}, 2: { ", {"a", "b"}, "1: a, 2: { "},
    };

    for (const auto& test : tests) {
        const auto got = dpso::str::format(test.str, test.args);
        if (got == test.expected)
            continue;

        test::failure(
            "str::format({}, {}): "
            "expected {}, got {}",
            test::utils::toStr(test.str),
            test::utils::toStr(test.args),
            test::utils::toStr(test.expected),
            test::utils::toStr(got));
    }

    using dpso::str::format;

    // Test argument conversion.
    TEST_STR(
        format(
            "{} {} {} {} {} {}",
            1, 2.0f, 12.34f, 56.78, 'c', std::string{"str"}),
        "1 2 12.34 56.78 c str");
}


void testStr()
{
    testCmpIgnoreCase();
    testJustify();
    testToStr();
    testFormat();
}


}


REGISTER_TEST(testStr);
