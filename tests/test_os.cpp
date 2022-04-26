
#include "dpso_utils/os.h"

#include <string>
#include <vector>

#include "flow.h"
#include "utils.h"


static void testGetFileExt()
{
    struct Test {
        std::string path;
        std::string expectedExt;
    };

    std::vector<Test> tests{
        {"", ""},
        {".a", ""},
        {"a.ext", ".ext"},
        {"a.", ""},
    };

    for (const char* sep = dpsoDirSeparators; *sep; ++sep)
        tests.insert(tests.end(), {
            {std::string{"a.b"} + *sep, ""},
            {std::string{"a.b"} + *sep + ".a", ""},
            {std::string{"a.b"} + *sep + "a.ext", ".ext"},
            {std::string{"a.b"} + *sep + "a.", ""},
        });

    for (const auto& test : tests) {
        const auto* ext = dpsoGetFileExt(test.path.c_str());
        if (!ext) {
            if (!test.expectedExt.empty())
                test::failure(
                    "testGetFileExt(): dpsoGetFileExt(\"%s\"): "
                    "expected \"%s\", got null\n",
                    test::utils::escapeStr(test.path.c_str()).c_str(),
                    test::utils::escapeStr(
                        test.expectedExt.c_str()).c_str());

            continue;
        }

        if (ext == test.expectedExt)
            continue;

        std::string expected;
        if (test.expectedExt.empty())
            expected = "null";
        else
            expected = std::string{"\""} + test.expectedExt + '"';

        test::failure(
            "testGetFileExt(): dpsoGetFileExt(\"%s\"): "
            "expected \"%s\", got \"%s\"\n",
            test::utils::escapeStr(test.path.c_str()).c_str(),
            test::utils::escapeStr(expected.c_str()).c_str(),
            test::utils::escapeStr(ext).c_str());
    }
}


static void testOs()
{
    testGetFileExt();
}


REGISTER_TEST(testOs);
