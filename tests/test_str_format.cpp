
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "dpso_utils/str_format.h"

#include "flow.h"


static const char* argsToStr(
    std::initializer_list<DpsoFormatArg> args)
{
    static std::string result;
    result.clear();

    for (const auto& arg : args) {
        if (!result.empty())
            result += ", ";
        result += std::string("{") + arg.name + ": " + arg.str + '}';
    }

    return result.c_str();
}


static void checkStrNamedFormat(
    const char* str,
    std::initializer_list<DpsoFormatArg> args,
    const char* expected,
    int lineNum)
{
    const auto* got = dpsoStrNamedFormat(str, args);
    if (std::strcmp(got, expected) == 0)
        return;

    std::fprintf(
        stderr,
        "line %i: dpsoStrNamedFormat(\"%s\", %s): "
        "expected \"%s\", got \"%s\"\n",
        lineNum,
        str,
        argsToStr(args),
        expected,
        got);
    test::failure();
}


static void testStrNamedFormat()
{
    #define CHECK(str, args, expected) \
        checkStrNamedFormat(str, args, expected, __LINE__)
    using Args = std::initializer_list<DpsoFormatArg>;

    // Normal
    CHECK(
        "1: {a1}, 2: {a2}",
        (Args{{"a1", "v1"}, {"a2", "v2"}}),
        "1: v1, 2: v2");
    // Mention the same arg twice
    CHECK(
        "1: {a1}, 2: {a2}, 2: {a2}",
        (Args{{"a1", "v1"}, {"a2", "v2"}}),
        "1: v1, 2: v2, 2: v2");
    // Reorder
    CHECK(
        "2: {a2}, 1: {a1}",
        (Args{{"a1", "v1"}, {"a2", "v2"}}),
        "2: v2, 1: v1");

    // Nonexistent args
    CHECK(
        "1: {a1}, 3: {a3}, 1: {a1}",
        (Args{{"a1", "v1"}}),
        "1: v1, 3: {a3}, 1: v1");
    CHECK(
        "1: {a1}, 2: {}, 1: {a1}",
        (Args{{"a1", "v1"}}),
        "1: v1, 2: {}, 1: v1");

    // Brace substitution
    CHECK(
        "1: {{a1}}",
        (Args{{"a1", "v1"}}),
        "1: {a1}");
    // Stray }
    CHECK(
        "1: {a1}, 2: {{a2}, 1: {a1}",
        (Args{{"a1", "v1"}}),
        "1: v1, 2: {a2}, 1: {a1}");
    // { within name
    CHECK(
        "1: {a1}, 2: {a{2}, 1: {a1}",
        (Args{{"a1", "v1"}}),
        "1: v1, 2: {a{2}, 1: {a1}");
    // No closing }
    CHECK(
        "1: {a1}, 2: {a2",
        (Args{{"a1", "v1"}, {"a2", "v2"}}),
        "1: v1, 2: {a2");

    #undef CHECK
}


static void testStrFormat()
{
    testStrNamedFormat();
}


REGISTER_TEST(testStrFormat);
