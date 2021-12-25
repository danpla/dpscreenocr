
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

    for (const char* sep = dpsoDirSeparators; *sep; ++sep) {
        tests.insert(tests.end(), {
            {std::string{"a.b"} + *sep, ""},
            {std::string{"a.b"} + *sep + ".a", ""},
            {std::string{"a.b"} + *sep + "a.ext", ".ext"},
            {std::string{"a.b"} + *sep + "a.", ""},
        });
    }

    for (const auto& test : tests) {
        const auto* ext = dpsoGetFileExt(test.path.c_str());
        if (!ext) {
            if (!test.expectedExt.empty()) {
                std::fprintf(
                    stderr,
                    "testGetFileExt(): dpsoGetFileExt(\"%s\") "
                    "returned null while \"%s\" was expected\n",
                    test::utils::escapeStr(test.path.c_str()).c_str(),
                    test::utils::escapeStr(
                        test.expectedExt.c_str()).c_str());
                test::failure();
            }

            continue;
        }

        if (ext == test.expectedExt)
            continue;

        std::string expected;
        if (test.expectedExt.empty())
            expected = "null";
        else
            expected = std::string{"\""} + test.expectedExt + '"';

        std::fprintf(
            stderr,
            "testGetFileExt(): dpsoGetFileExt(\"%s\") "
            "returned \"%s\" while %s was expected\n",
            test::utils::escapeStr(test.path.c_str()).c_str(),
            test::utils::escapeStr(ext).c_str(),
            test::utils::escapeStr(expected.c_str()).c_str());
        test::failure();
    }
}


static void testOs()
{
    testGetFileExt();
}


REGISTER_TEST(testOs);
