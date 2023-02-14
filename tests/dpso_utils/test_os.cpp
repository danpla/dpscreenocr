
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

#include "dpso_utils/os.h"

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


const auto* const testUnicodeFileName =
    // 汉语.txt
    "\346\261\211\350\257\255.txt";


static void testFopen()
{
    dpso::StdFileUPtr fp{dpsoFopen(testUnicodeFileName, "wb")};
    if (!fp)
        test::failure(
            "dpsoFopen(\"%s\"): %s\n",
            testUnicodeFileName,
            std::strerror(errno));

    dpsoRemove(testUnicodeFileName);
}


static void testRemove()
{
    {
        dpso::StdFileUPtr fp{dpsoFopen(testUnicodeFileName, "wb")};
        if (!fp)
            test::fatalError(
                "testRemove: dpsoFopen(\"%s\"): %s\n",
                testUnicodeFileName,
                std::strerror(errno));
    }

    if (dpsoRemove(testUnicodeFileName) != 0)
        test::failure(
            "dpsoRemove(\"%s\"): %s\n",
            testUnicodeFileName,
            std::strerror(errno));
}


static void testSyncFile()
{
    const auto* fileName = "test_sync_file.txt";

    dpso::StdFileUPtr fp{dpsoFopen(fileName, "wb")};
    if (!fp)
        test::fatalError(
            "testSyncFile: dpsoFopen(\"%s\"): %s\n",
            fileName,
            std::strerror(errno));

    if (!dpsoSyncFile(fp.get()))
        test::failure(
            "testSyncFile: dpsoSyncFile(): %s\n",
            std::strerror(errno));
}


void testSyncFileDir()
{
    const auto* fileName = "test_sync_file.txt";

    test::utils::saveText("testSyncFileDir", fileName, "");

    if (!dpsoSyncFileDir(fileName))
        test::fatalError(
            "testSyncFileDir: dpsoSyncFileDir(\"%s\"): %s\n",
            fileName,
            std::strerror(errno));

    dpsoRemove(fileName);
}


static void testOs()
{
    testGetFileExt();
    testFopen();
    testRemove();
    testSyncFile();
    testSyncFileDir();
}


REGISTER_TEST(testOs);
