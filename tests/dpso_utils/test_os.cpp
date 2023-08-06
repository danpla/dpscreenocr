
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

    for (const char* sep = dpso::os::dirSeparators; *sep; ++sep)
        tests.insert(tests.end(), {
            {std::string{"a.b"} + *sep, ""},
            {std::string{"a.b"} + *sep + ".a", ""},
            {std::string{"a.b"} + *sep + "a.ext", ".ext"},
            {std::string{"a.b"} + *sep + "a.", ""},
        });

    for (const auto& test : tests) {
        const auto* ext = dpso::os::getFileExt(test.path.c_str());
        if (!ext) {
            if (!test.expectedExt.empty())
                test::failure(
                    "testGetFileExt(): os::getFileExt(\"%s\"): "
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
            "testGetFileExt(): os::getFileExt(\"%s\"): "
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
    {
        dpso::os::StdFileUPtr fp{
            dpso::os::fopen(testUnicodeFileName, "wb")};
        if (!fp)
            test::failure(
                "os::fopen(\"%s\"): %s\n",
                testUnicodeFileName,
                std::strerror(errno));
    }

    test::utils::removeFile(testUnicodeFileName);
}


static void testRemoveFile()
{
    test::utils::saveText(
        "testRemoveFile", testUnicodeFileName, "abc");

    try {
        dpso::os::removeFile(testUnicodeFileName);
    } catch (dpso::os::Error& e) {
        test::failure(
            "os::removeFile(\"%s\"): %s\n",
            testUnicodeFileName,
            e.what());
    }
}


static void testSyncFile()
{
    const auto* fileName = "test_sync_file.txt";

    dpso::os::StdFileUPtr fp{dpso::os::fopen(fileName, "wb")};
    if (!fp)
        test::fatalError(
            "testSyncFile: os::fopen(\"%s\"): %s\n",
            fileName, std::strerror(errno));

    try {
        dpso::os::syncFile(fp.get());
    } catch (dpso::os::Error& e) {
        test::failure(
            "testSyncFile: os::syncFile(): %s\n", e.what());
    }
}


void testSyncFileDir()
{
    const auto* fileName = "test_sync_file.txt";

    test::utils::saveText("testSyncFileDir", fileName, "");

    try {
        dpso::os::syncFileDir(fileName);
    } catch (dpso::os::Error& e) {
        test::fatalError(
            "testSyncFileDir: os::syncFileDir(\"%s\"): %s\n",
            fileName, e.what());
    }

    test::utils::removeFile(fileName);
}


static void testOs()
{
    testGetFileExt();
    testFopen();
    testRemoveFile();
    testSyncFile();
    testSyncFileDir();
}


REGISTER_TEST(testOs);
