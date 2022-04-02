
#include <cstring>
#include <initializer_list>
#include <string>

#include "dpso_utils/str_format.h"

#include "flow.h"


static std::string argsToStr(
    std::initializer_list<DpsoFormatArg> args)
{
    std::string result;

    for (const auto& arg : args) {
        if (!result.empty())
            result += ", ";
        result += std::string("{") + arg.name + ": " + arg.str + '}';
    }

    return result;
}


static void testStrFormat()
{
    struct Test {
        const char* str;
        std::initializer_list<DpsoFormatArg> args;
        const char* expected;
    };

    const Test tests[] = {
        {
            // Normal
            "1: {a1}, 2: {a2}",
            {{"a1", "v1"}, {"a2", "v2"}},
            "1: v1, 2: v2"
        },
        {
            // Mention the same arg twice
            "1: {a1}, 2: {a2}, 2: {a2}",
            {{"a1", "v1"}, {"a2", "v2"}},
            "1: v1, 2: v2, 2: v2"
        },
        {
            // Reverse order
            "2: {a2}, 1: {a1}",
            {{"a1", "v1"}, {"a2", "v2"}},
            "2: v2, 1: v1"
        },
        {
            // Nonexistent arg
            "1: {a1}, 3: {???}, 1: {a1}",
            {{"a1", "v1"}},
            "1: v1, 3: {???}, 1: v1"
        },
        {
            // No name within {}
            "1: {a1}, 2: {}, 1: {a1}",
            {{"a1", "v1"}},
            "1: v1, 2: {}, 1: v1"
        },
        {
            // Brace substitution
            "1: {{a1}}",
            {{"a1", "v1"}},
            "1: {a1}"
        },
        {
            // Stray }
            "1: {a1}, 2: {{a2}, 1: {a1}",
            {{"a1", "v1"}},
            "1: v1, 2: {a2}, 1: {a1}"
        },
        {
            // { within name
            "1: {a1}, 2: {a{2}, 1: {a1}",
            {{"a1", "v1"}},
            "1: v1, 2: {a{2}, 1: {a1}"
        },
        {
            // No closing }
            "1: {a1}, 2: {a2",
            {{"a1", "v1"}, {"a2", "v2"}},
            "1: v1, 2: {a2"
        },
    };

    for (const auto& test : tests) {
        const auto* got = dpsoStrNamedFormat(test.str, test.args);
        if (std::strcmp(got, test.expected) == 0)
            continue;

        test::failure(
            "dpsoStrNamedFormat(\"%s\", %s): "
            "expected \"%s\", got \"%s\"\n",
            test.str,
            argsToStr(test.args).c_str(),
            test.expected,
            got);
    }
}


REGISTER_TEST(testStrFormat);
