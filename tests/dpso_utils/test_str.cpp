
#include "dpso_utils/str.h"

#include "flow.h"


namespace {


enum class Order {
    less,
    equal,
    greater
};


}


static Order getOrder(int cmpResult)
{
    if (cmpResult < 0)
        return Order::less;

    if (cmpResult == 0)
        return Order::equal;

    return Order::greater;
}


static const char* orderToStr(Order order)
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


static void testCmpSubStr()
{
    using namespace dpso::str;

    struct Test {
        const char* str;
        const char* subStr;
        std::size_t subStrLen;
        unsigned cmpOptions;
        Order expectedOrder;
    };

    const Test tests[] = {
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
            "testCmpSubStr: cmpSubStr(\"%s\", \"%s\", %zu, %u): "
            "expected %s, got %s\n",
            test.str, test.subStr, test.subStrLen, test.cmpOptions,
            orderToStr(test.expectedOrder), orderToStr(gotOrder));
    }
}


static void testPrintf()
{
    using namespace dpso;

    #define TEST(CALL, EXPECTED) \
        do { \
        const auto got = CALL; \
        if (got != EXPECTED) \
            test::failure( \
                "testPrintf: %s: " \
                "expected \"%s\", got \"%s\"\n", \
                #CALL, EXPECTED, got.c_str()); \
        } while (false)


    TEST(str::printf("%s", ""), "");
    TEST(str::printf("%s %i", "Str", 123), "Str 123");

    const std::string longStr(4096, 'a');
    TEST(str::printf("%s", longStr.c_str()), longStr.c_str());

    #undef TEST
}


static void testStr()
{
    testCmpSubStr();
    testPrintf();
}


REGISTER_TEST(testStr);
