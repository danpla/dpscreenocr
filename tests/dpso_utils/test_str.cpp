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


std::string_view toStr(Order order)
{
    switch (order) {
    case Order::less:
        return "<";
    case Order::equal:
        return "==";
    case Order::greater:
        return ">";
    }

    return {};
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
    std::string_view callStr,
    std::string_view callResult,
    std::string_view expected,
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
    using namespace dpso::str;

    TEST_STR(justifyLeft("x", 4), "x   ");
    TEST_STR(justifyRight("x", 4), "   x");

    TEST_STR(justifyLeft("x", 4, '-'), "x---");
    TEST_STR(justifyRight("x", 4, '-'), "---x");

    TEST_STR(justifyLeft("abcd", 2), "abcd");
    TEST_STR(justifyRight("abcd", 2), "abcd");
}


void testTrim()
{
    using namespace dpso::str;

    TEST_STR(trimLeft(" ", isSpace), "");
    TEST_STR(trimLeft(" a b ", isSpace), "a b ");

    TEST_STR(trimRight(" ", isSpace), "");
    TEST_STR(trimRight(" a b ", isSpace), " a b");

    TEST_STR(trim(" ", isSpace), "");
    TEST_STR(trim(" a b ", isSpace), "a b");
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
        "-1" + std::string(63, '0'));
}


void testFormat()
{
    using namespace dpso;

    const struct {
        std::string_view fmt;
        std::initializer_list<std::string_view> args;
        std::string_view expected;
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
        // } }
        {"1: } }, 2: {}", {"a", "b"}, "1: } }, 2: {}"},
        // { {
        {"1: { {, 2: {}", {"a", "b"}, "1: { {, 2: {}"},
    };

    for (const auto& test : tests) {
        const auto got = str::format(test.fmt, test.args);
        if (got == test.expected)
            continue;

        test::failure(
            "str::format({}, {}): "
            "expected {}, got {}",
            test::utils::toStr(test.fmt),
            test::utils::toStr(test.args),
            test::utils::toStr(test.expected),
            test::utils::toStr(got));
    }

    // Test argument conversion.
    TEST_STR(
        str::format(
            "{} {} {} {} {} {}",
            1, 2.0f, 12.34f, 56.78, 'c', std::string{"str"}),
        "1 2 12.34 56.78 c str");
}


void testStr()
{
    testCmpIgnoreCase();
    testJustify();
    testTrim();
    testToStr();
    testFormat();
}


}


REGISTER_TEST(testStr);
