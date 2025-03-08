#include <cerrno>
#include <string>
#include <vector>

#include "dpso_utils/os.h"
#include "dpso_utils/scope_exit.h"

#include "flow.h"
#include "utils.h"


using namespace dpso;


namespace {


void testPathSplit()
{
    const struct {
        const char* path;
        const char* dirName;
        const char* baseName;
    } tests[]{
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
        const auto dirName = os::getDirName(test.path);
        if (dirName != test.dirName)
            test::failure(
                "os::getDirName(\"{}\"): Expected \"{}\", got \"{}\"",
                test.path, test.dirName, dirName);

        const auto baseName = os::getBaseName(test.path);
        if (baseName != test.baseName)
            test::failure(
                "os::getBaseName(\"{}\"): Expected \"{}\", got "
                "\"{}\"",
                test.path, test.baseName, baseName);
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

    for (const char* sep = os::dirSeparators; *sep; ++sep)
        tests.insert(tests.end(), {
            {std::string{"a.b"} + *sep, ""},
            {std::string{"a.b"} + *sep + ".a", ""},
            {std::string{"a.b"} + *sep + "a.ext", ".ext"},
            {std::string{"a.b"} + *sep + "a.", ""},
        });

    for (const auto& test : tests) {
        const auto* ext = os::getFileExt(test.path.c_str());
        if (!ext) {
            if (!test.expectedExt.empty())
                test::failure(
                    "os::getFileExt(): expected {}, got null",
                    test::utils::toStr(test.path),
                    test::utils::toStr(test.expectedExt));

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
            "os::getFileExt({}): expected {}, got {}",
            test::utils::toStr(test.path),
            test::utils::toStr(expected),
            test::utils::toStr(ext));
    }
}


template<typename Fn>
bool checkFileNotFoundError(Fn fn, const char* fnName)
{
    try {
        fn();
        test::failure(
            "{}() for a nonexistent file didn't threw an error",
            fnName);
    } catch (os::FileNotFoundError&) {
        return true;
    } catch (os::Error& e) {
        test::failure(
            "{}() for a nonexistent file threw an error "
            "(\"{}\") of class other than FileNotFoundError",
            fnName, e.what());
    }

    return false;
}


#define CHECK_FILE_NOT_FOUND_ERROR(FN, ...) \
    checkFileNotFoundError([]{ FN(__VA_ARGS__); }, #FN)


void testGetFileSize()
{
    const auto* fileName = "test_get_file_size.txt";
    const std::int64_t size = 123456;

    test::utils::saveText(
        "testGetFileSize", fileName, std::string(size, '1').c_str());

    try {
        const auto gotSize = os::getFileSize(fileName);
        if (gotSize != size)
            test::failure(
                "os::getFileSize(\"{}\"): expected {}, got {}",
                fileName, size, gotSize);
    } catch (os::Error& e) {
        test::failure(
            "os::getFileSize(\"{}\"): {}", fileName, e.what());
    }

    test::utils::removeFile(fileName);

    CHECK_FILE_NOT_FOUND_ERROR(os::getFileSize, "nonexistent_file");
}


void testResizeFile(
    const char* actionName,
    const char* fileName,
    std::int64_t newSize)
{
    try {
        os::resizeFile(fileName, newSize);
    } catch (os::Error& e) {
        test::failure(
            "os::resizeFile(\"{}\", {}) ({}): {}",
            fileName, newSize, actionName, e.what());
    }

    std::int64_t actualSize{};
    try {
        actualSize = os::getFileSize(fileName);
    } catch (os::Error& e) {
        test::fatalError(
            "testResizeFile: os::getFileSize(\"{}\"): {}",
            fileName, e.what());
    }

    if (actualSize != newSize)
        test::failure(
            "os::resizeFile(\"{}\", {}) ({}): Unexpected size ({}) "
            "after resizing",
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

    CHECK_FILE_NOT_FOUND_ERROR(
        os::resizeFile, "nonexistent_file", 10);
}


const auto* const testUnicodeFileName =
    // 汉语.txt
    "\346\261\211\350\257\255.txt";


void testRemoveFile()
{
    test::utils::saveText(
        "testRemoveFile", testUnicodeFileName, "abc");

    try {
        os::removeFile(testUnicodeFileName);
    } catch (os::Error& e) {
        test::failure(
            "os::removeFile(\"{}\"): {}",
            testUnicodeFileName, e.what());
    }

    CHECK_FILE_NOT_FOUND_ERROR(os::removeFile, "nonexistent_file");
}


void testReplaceSrcExists(bool dstExists)
{
    const auto* srcFilePath = "test_replace_src.txt";
    const auto* dstFilePath = "test_replace_dst.txt";

    test::utils::saveText("testReplace", srcFilePath, srcFilePath);
    if (dstExists)
        test::utils::saveText(
            "testReplace", dstFilePath, dstFilePath);

    const ScopeExit scopeExit{
        [=]
        {
            test::utils::removeFile(srcFilePath);
            test::utils::removeFile(dstFilePath);
        }};

    try {
        os::replace(srcFilePath, dstFilePath);
    } catch (os::Error& e) {
        test::failure(
            "os::replace(\"{}\", \"{}\"): {}",
            srcFilePath, dstFilePath, e.what());
        return;
    }

    try {
        (void)os::getFileSize(srcFilePath);
        test::failure(
            "os::replace(\"{}\", \"{}\") didn't fail, but the source "
            "file still exists",
            srcFilePath, dstFilePath);
    } catch (os::Error&) {
    }

    if (!dstExists)
        try {
            (void)os::getFileSize(dstFilePath);
        } catch (os::FileNotFoundError&) {
            test::failure(
                "os::replace(\"{}\", \"{}\") didn't create the "
                "destination file",
                srcFilePath, dstFilePath);
            return;
        } catch (os::Error&) {
        }

    const auto dstText = test::utils::loadText(
        "testReplace", dstFilePath);

    if (dstText != srcFilePath)
        test::failure(
            "os::replace(\"{}\", \"{}\"): the destination file "
            "contents (\"{}\") are not the same as in source "
            "(\"{}\")",
            srcFilePath, dstFilePath, dstText, srcFilePath);
}


void testReplaceNoSrc()
{
    CHECK_FILE_NOT_FOUND_ERROR(
        os::replace, "nonexistent_file", "dst");
}


void testReplace()
{
    testReplaceSrcExists(false);
    testReplaceSrcExists(true);
    testReplaceNoSrc();
}


void testSyncDir()
{
    const auto* dirPath = ".";

    try {
        os::syncDir(dirPath);
    } catch (os::Error& e) {
        test::failure("os::syncDir(\"{}\"): {}", dirPath, e.what());
    }
}


void testLoadData()
{
    const std::string filePath{"test_load_data.txt"};

    test::utils::saveText(
        "testLoadData", filePath.c_str(), filePath.c_str());

    try {
        const auto data = os::loadData(filePath.c_str());
        if (data != filePath)
            test::failure(
                "os::loadData(\"{}\"): expected \"{}\", got \"{}\"",
                filePath, data, filePath);
    } catch (os::Error& e) {
        test::failure("os::loadData(\"{}\"): {}", filePath, e.what());
    }

    test::utils::removeFile(filePath.c_str());

    CHECK_FILE_NOT_FOUND_ERROR(os::loadData, "nonexistent_file");
}


void testOs()
{
    testPathSplit();
    testGetFileExt();
    testGetFileSize();
    testResizeFile();
    testRemoveFile();
    testReplace();
    testSyncDir();
    testLoadData();
}


}


REGISTER_TEST(testOs);
