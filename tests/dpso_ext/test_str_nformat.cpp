
#include <cstring>
#include <initializer_list>
#include <string>

#include "dpso_ext/str_nformat.h"
#include "dpso_utils/str.h"

#include "flow.h"


static std::string argsToStr(
    std::initializer_list<DpsoStrNFormatArg> args)
{
    std::string result = "{";

    auto firstArg{true};
    for (const auto& arg: args) {
        if (!firstArg)
            result += ", ";

        result += dpso::str::printf("{%s: %s}", arg.name, arg.str);

        firstArg = false;
    }

    result += '}';

    return result;
}


static void testStrNFormat()
{
    const struct Test {
        const char* str;
        std::initializer_list<DpsoStrNFormatArg> args;
        const char* expected;
    } tests[] = {
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
        const auto* got = dpsoStrNFormat(test.str, test.args);
        if (std::strcmp(got, test.expected) == 0)
            continue;

        test::failure(
            "dpsoStrNFormat(\"%s\", %s): "
            "expected \"%s\", got \"%s\"\n",
            test.str,
            argsToStr(test.args).c_str(),
            test.expected,
            got);
    }
}


REGISTER_TEST(testStrNFormat);
