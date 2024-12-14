
#include "dpso_utils/version_cmp.h"

#include "flow.h"
#include "utils.h"


static void testVersionCmp()
{
    const struct {
        const char* strA;
        const char* strB;
        bool isLess;
    } tests [] = {
        {"", "1", true},
        {"1", "", false},

        {"1.0", "1.0.1", true},
        {"1.0.1", "1.0", false},

        {"1.0", "1.1", true},
        {"1.1", "1.0", false},

        {"1.1", "1.10", true},
        {"1.10", "1.1", false},

        {"1.2.3", "1.2.3.1", true},
        {"1.2.3.1", "1.2.3", false},

        {"1.2.3", "01.002.0003", false},
        {"01.002.0003", "1.2.3", false},

        {"1.0-rc1", "1.0-rc1", false},

        {"1.1-rc1", "1.0-rc2", false},
        {"1.0-rc2", "1.1-rc1", true},

        {"1.0-rc1", "1.0-rc2", true},
        {"1.0-rc2", "1.0-rc1", false},

        {"1.0-rc1", "1.0", true},
        {"1.0", "1.0-rc1", false},
    };

    for (const auto& test : tests) {
        const dpso::VersionCmp a{test.strA};
        const dpso::VersionCmp b{test.strB};

        const auto isLess = a < b;
        if (isLess == test.isLess)
            continue;

        test::failure(
            "\"{}\" < \"{}\" expected to be {}\n",
            test.strA,
            test.strB,
            test::utils::toStr(test.isLess));
    }
}


REGISTER_TEST(testVersionCmp);
