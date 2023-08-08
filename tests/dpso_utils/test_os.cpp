
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

#include "dpso_utils/os.h"

#include "flow.h"
#include "utils.h"


static void testPathSplit()
{
    const struct Test {
        const char* path;
        const char* dirName;
        const char* baseName;
    } tests[] = {
        #ifdef _WIN32

        {"", "", ""},
        {"c:", "c:", ""},
        {"c:\\", "c:\\", ""},
        {"c:\\a", "c:\\", "a"},
        {"c:\\a\\", "c:\\a", ""},
        {"c:\\\\a", "c:\\\\", "a"},
        {"c:\\\\a\\\\b", "c:\\\\a", "b"},
        {"c:/a", "c:/", "a"},
        {"c:/a/", "c:/a", ""},
        {"c:/a/b", "c:/a", "b"},

        // UNC paths.
        {"\\\\", "\\\\", ""},
        {"\\\\server", "\\\\server", ""},
        {"\\\\server\\", "\\\\server\\", ""},
        {"\\\\server\\share", "\\\\server\\share", ""},
        {"\\\\server\\share\\", "\\\\server\\share\\", ""},
        {"\\\\server\\share\\a", "\\\\server\\share\\", "a"},
        {"\\\\server\\share\\a\\b", "\\\\server\\share\\a", "b"},

        // Device paths.
        {"\\\\?", "\\\\?", ""},
        {"\\\\?\\", "\\\\?\\", ""},
        {"\\\\?\\c:", "\\\\?\\c:", ""},
        {"\\\\?\\c:\\\\", "\\\\?\\c:\\\\", ""},
        {"\\\\?\\c:\\\\a", "\\\\?\\c:\\\\", "a"},
        {"\\\\?\\c:\\\\a\\\\b", "\\\\?\\c:\\\\a", "b"},
        {"\\\\?\\UNC", "\\\\?\\UNC", ""},
        {"\\\\?\\UNC\\", "\\\\?\\UNC\\", ""},
        {"\\\\?\\UNC\\server", "\\\\?\\UNC\\server", ""},
        {"\\\\?\\UNC\\server\\", "\\\\?\\UNC\\server\\", ""},
        {
            "\\\\?\\UNC\\server\\share",
            "\\\\?\\UNC\\server\\share",
            ""
        },
        {
            "\\\\?\\UNC\\server\\share\\",
            "\\\\?\\UNC\\server\\share\\",
            ""
        },
        {
            "\\\\?\\UNC\\server\\share\\a",
            "\\\\?\\UNC\\server\\share\\",
            "a"
        },
        {
            "\\\\?\\UNC\\server\\share\\a\\b",
            "\\\\?\\UNC\\server\\share\\a",
            "b"
        },
        {
            "\\\\?\\unC\\server\\share",
            "\\\\?\\unC\\server\\share",
            ""
        },

        #else

        {"", "", ""},
        {"/", "/", ""},
        {"//", "//", ""},
        {"//a", "//", "a"},
        {"/a/", "/a", ""},
        {"/a//", "/a", ""},
        {"/a/b", "/a", "b"},
        {"/a//b", "/a", "b"},
        {"a", "", "a"},
        {"a/b", "a", "b"},

        #endif
    };

    for (const auto& test : tests) {
        const auto dirName = dpso::os::getDirName(test.path);
        if (dirName != test.dirName)
            test::failure(
                "os::getDirName(\"%s\"): Expected \"%s\", got "
                "\"%s\"\n",
                test.path,
                test.dirName,
                dirName.c_str());

        const auto baseName = dpso::os::getBaseName(test.path);
        if (baseName != test.baseName)
            test::failure(
                "os::getBaseName(\"%s\"): Expected \"%s\", got "
                "\"%s\"\n",
                test.path,
                test.baseName,
                baseName.c_str());
    }
}


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
    dpso::os::StdFileUPtr fp{
        dpso::os::fopen(testUnicodeFileName, "wb")};
    if (!fp) {
        test::failure(
            "os::fopen(\"%s\"): %s\n",
            testUnicodeFileName,
            std::strerror(errno));
        return;
    }

    fp.reset();
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

    fp.reset();
    test::utils::removeFile(fileName);
}


void testSyncFileDir()
{
    const auto* dirPath = ".";

    try {
        dpso::os::syncDir(dirPath);
    } catch (dpso::os::Error& e) {
        test::fatalError(
            "testSyncFileDir: os::syncDir(\"%s\"): %s\n",
            dirPath, e.what());
    }
}


static void testOs()
{
    testPathSplit();
    testGetFileExt();
    testFopen();
    testRemoveFile();
    testSyncFile();
    testSyncFileDir();
}


REGISTER_TEST(testOs);
