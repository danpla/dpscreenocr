
#include "dpso_utils/strftime.h"

#include "flow.h"


static void testStrftime()
{
    std::tm tm{};
    tm.tm_year = 2022 - 1900;
    tm.tm_mon = 10;
    tm.tm_mday = 17;
    tm.tm_hour = 23;
    tm.tm_min = 58;
    tm.tm_sec = 59;

    const struct Test {
        const char* format;
        const char* result;
    } tests[] = {
        {"", ""},
        {"%Y-%m-%d %H:%M:%S", "2022-11-17 23:58:59"},
    };

    for (const auto& test : tests) {
        const auto result = dpso::strftime(test.format, &tm);
        if (result != test.result)
            test::failure(
                "dpso::strftime(\"{}\"): "
                "Expected \"{}\", got \"{}\"\n",
                test.format,
                test.result,
                result);
    }
}


REGISTER_TEST(testStrftime);
