
#include <cerrno>
#include <string>
#include <vector>

#include "dpso_utils/os.h"
#include "dpso_utils/scope_exit.h"

#include "flow.h"
#include "utils.h"


namespace {


void testPathSplit()
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
            ""},
        {
            "\\\\?\\UNC\\server\\share\\",
            "\\\\?\\UNC\\server\\share\\",
            ""},
        {
            "\\\\?\\UNC\\server\\share\\a",
            "\\\\?\\UNC\\server\\share\\",
            "a"},
        {
            "\\\\?\\UNC\\server\\share\\a\\b",
            "\\\\?\\UNC\\server\\share\\a",
            "b"},
        {
            "\\\\?\\unC\\server\\share",
            "\\\\?\\unC\\server\\share",
            ""},

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
                "os::getDirName(\"{}\"): Expected \"{}\", got "
                "\"{}\"\n",
                test.path,
                test.dirName,
                dirName);

        const auto baseName = dpso::os::getBaseName(test.path);
        if (baseName != test.baseName)
            test::failure(
                "os::getBaseName(\"{}\"): Expected \"{}\", got "
                "\"{}\"\n",
                test.path,
                test.baseName,
                baseName);
    }
}


void testGetFileExt()
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
                    "os::getFileExt(\"{}\"): expected \"{}\", "
                    "got null\n",
                    test::utils::escapeStr(test.path),
                    test::utils::escapeStr(test.expectedExt));

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
            "os::getFileExt(\"{}\"): expected \"{}\", got \"{}\"\n",
            test::utils::escapeStr(test.path),
            test::utils::escapeStr(expected),
            test::utils::escapeStr(ext));
    }
}


void testGetFileSize()
{
    const auto* fileName = "test_get_file_size.txt";
    const std::int64_t size = 123456;

    test::utils::saveText(
        "testGetFileSize", fileName, std::string(size, '1').c_str());

    try {
        const auto gotSize = dpso::os::getFileSize(fileName);
        if (gotSize != size)
            test::failure(
                "os::getFileSize(\"{}\"): expected {}, got {}\n",
                fileName, size, gotSize);
    } catch (dpso::os::Error& e) {
        test::failure(
            "os::getFileSize(\"{}\"): {}\n", fileName, e.what());
    }

    test::utils::removeFile(fileName);

    try {
        dpso::os::getFileSize("nonexistent_file");
        test::failure(
            "os::getFileSize() for a nonexistent source file didn't "
            "threw an error");
    } catch (dpso::os::FileNotFoundError&) {
    } catch (dpso::os::Error& e) {
        test::failure(
            "os::getFileSize() for a nonexistent file threw an error "
            "(\"{}\") of class other than FileNotFoundError\n",
            e.what());
    }
}


void testResizeFile(
    const char* actionName,
    const char* fileName,
    std::int64_t newSize)
{
    try {
        dpso::os::resizeFile(fileName, newSize);
    } catch (dpso::os::Error& e) {
        test::failure(
            "os::resizeFile(\"{}\", {}) ({}): {}\n",
            fileName, newSize, actionName, e.what());
    }

    std::int64_t actualSize{};
    try {
        actualSize = dpso::os::getFileSize(fileName);
    } catch (dpso::os::Error& e) {
        test::fatalError(
            "testResizeFile: os::getFileSize(\"{}\"): {}",
            fileName, e.what());
    }

    if (actualSize != newSize)
        test::failure(
            "os::resizeFile(\"{}\", {}) ({}): Unexpected size ({}) "
            "after resizing\n",
            fileName, newSize, actionName, actualSize);
}


void testResizeFile()
{
    const auto* fileName = "test_resize_file.txt";
    test::utils::saveText(
        "testResizeFile", fileName, std::string(10, '1').c_str());

    testResizeFile("shrink", fileName, 5);
    testResizeFile("expand", fileName, 20);

    test::utils::removeFile(fileName);

    try {
        dpso::os::resizeFile("nonexistent_file", 10);
        test::failure(
            "os::resizeFile() for a nonexistent file didn't threw an "
            "error\n");
    } catch (dpso::os::FileNotFoundError&) {
    } catch (dpso::os::Error& e) {
        test::failure(
            "os::resizeFile() for a nonexistent file threw an error "
            "(\"{}\") of class other than FileNotFoundError\n",
            e.what());
    }
}


const auto* const testUnicodeFileName =
    // 汉语.txt
    "\346\261\211\350\257\255.txt";


void testFopen()
{
    dpso::os::StdFileUPtr fp{
        dpso::os::fopen(testUnicodeFileName, "wb")};
    if (!fp) {
        test::failure(
            "os::fopen(\"{}\"): {}\n",
            testUnicodeFileName,
            dpso::os::getErrnoMsg(errno));
        return;
    }

    fp.reset();
    test::utils::removeFile(testUnicodeFileName);
}


void testRemoveFile()
{
    test::utils::saveText(
        "testRemoveFile", testUnicodeFileName, "abc");

    try {
        dpso::os::removeFile(testUnicodeFileName);
    } catch (dpso::os::Error& e) {
        test::failure(
            "os::removeFile(\"{}\"): {}\n",
            testUnicodeFileName,
            e.what());
    }

    try {
        dpso::os::removeFile("nonexistent_file");
        test::failure(
            "os::removeFile() for a nonexistent file didn't threw an "
            "error\n");
    } catch (dpso::os::FileNotFoundError&) {
    } catch (dpso::os::Error& e) {
        test::failure(
            "os::removeFile() for a nonexistent file threw an error "
            "(\"{}\") of class other than FileNotFoundError\n",
            e.what());
    }
}


void testReplaceSrcExists(bool dstExists)
{
    const auto* srcFilePath = "test_replace_src.txt";
    const auto* dstFilePath = "test_replace_dst.txt";

    test::utils::saveText("testReplace", srcFilePath, srcFilePath);
    if (dstExists)
        test::utils::saveText(
            "testReplace", dstFilePath, dstFilePath);

    const dpso::ScopeExit scopeExit{
        [=]
        {
            test::utils::removeFile(srcFilePath);
            test::utils::removeFile(dstFilePath);
        }};

    try {
        dpso::os::replace(srcFilePath, dstFilePath);
    } catch (dpso::os::Error& e) {
        test::failure(
            "os::replace(\"{}\", \"{}\"): {}\n",
            srcFilePath, dstFilePath, e.what());
        return;
    }

    if (dpso::os::StdFileUPtr{dpso::os::fopen(srcFilePath, "r")})
        test::failure(
            "os::replace(\"{}\", \"{}\") didn't failed, but the "
            "source file still exists\n",
            srcFilePath, dstFilePath);

    if (!dstExists
            && !dpso::os::StdFileUPtr{
                dpso::os::fopen(dstFilePath, "r")}) {
        test::failure(
            "os::replace(\"{}\", \"{}\") didn't created the "
            "destination file\n",
            srcFilePath, dstFilePath);
        return;
    }

    const auto dstText = test::utils::loadText(
        "testReplace", dstFilePath);

    if (dstText != srcFilePath)
        test::failure(
            "os::replace(\"{}\", \"{}\"): the destination file "
            "contents (\"{}\") are not the same as in source "
            "(\"{}\")\n",
            srcFilePath, dstFilePath, dstText, srcFilePath);
}


void testReplaceNoSrc()
{
    try {
        dpso::os::replace("nonexistent_file", "dst");
        test::failure(
            "os::replace() for a nonexistent source file didn't "
            "threw an error");
    } catch (dpso::os::FileNotFoundError&) {
    } catch (dpso::os::Error& e) {
        test::failure(
            "os::replace() for a nonexistent file threw an error "
            "(\"{}\") of class other than FileNotFoundError\n",
            e.what());
    }
}


void testReplace()
{
    testReplaceSrcExists(false);
    testReplaceSrcExists(true);
    testReplaceNoSrc();
}


void testSyncFile()
{
    const auto* fileName = "test_sync_file.txt";

    dpso::os::StdFileUPtr fp{dpso::os::fopen(fileName, "wb")};
    if (!fp)
        test::fatalError(
            "testSyncFile: os::fopen(\"{}\"): {}\n",
            fileName, dpso::os::getErrnoMsg(errno));

    try {
        dpso::os::syncFile(fp.get());
    } catch (dpso::os::Error& e) {
        test::failure("os::syncFile(): {}\n", e.what());
    }

    fp.reset();
    test::utils::removeFile(fileName);
}


void testSyncDir()
{
    const auto* dirPath = ".";

    try {
        dpso::os::syncDir(dirPath);
    } catch (dpso::os::Error& e) {
        test::failure("os::syncDir(\"{}\"): {}\n", dirPath, e.what());
    }
}


void testOs()
{
    testPathSplit();
    testGetFileExt();
    testGetFileSize();
    testResizeFile();
    testFopen();
    testRemoveFile();
    testReplace();
    testSyncFile();
    testSyncDir();
}


}


REGISTER_TEST(testOs);
