
#include <cerrno>
#include <cinttypes>
#include <cstring>
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


void testGetFileSize()
{
    const auto* fileName = "test_get_file_size.txt";
    const std::int64_t size = 123456;

    test::utils::saveText(
        "testGetFileSize", fileName, std::string(size, '1').c_str());

    const auto gotSize = dpso::os::getFileSize(fileName);
    if (gotSize != size)
        test::failure(
            "os::getFileSize(\"%s\"): expected \"%" PRIi64 "\", got "
            "\"%" PRIi64 "\"\n",
            fileName, size, gotSize);

    test::utils::removeFile(fileName);
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
            "os::fopen(\"%s\"): %s\n",
            testUnicodeFileName,
            std::strerror(errno));
        return;
    }

    fp.reset();
    test::utils::removeFile(testUnicodeFileName);
}


void testReadLine()
{
    const struct Test {
        const char* text;
        std::vector<std::string> expectedLines;
    } tests[] = {
        {"", {}},
        {" ", {" "}},
        {"\r", {""}},
        {"\n", {""}},
        {"\r\n", {""}},
        {"\n\r", {"", ""}},
        {"\r\r", {"", ""}},
        {"\n\n", {"", ""}},
        {"a", {"a"}},
        {"a\n", {"a"}},
        {"a\nb\r", {"a", "b"}},
        {"a\nb\rc", {"a", "b", "c"}},
        {"a\nb\rc\r\n", {"a", "b", "c"}},
    };

    const auto* fileName = "test_read_line.txt";

    for (const auto& test : tests) {
        test::utils::saveText(
            "testReadLine", fileName, test.text);

        dpso::os::StdFileUPtr fp{
            dpso::os::fopen(fileName, "rb")};
        if (!fp)
            test::fatalError(
                "os::fopen(\"%s\"): %s\n",
                fileName,
                std::strerror(errno));

        std::string line{"initial line content"};

        std::vector<std::string> lines;
        while (dpso::os::readLine(fp.get(), line))
            lines.push_back(line);

        if (!line.empty())
            test::failure(
                "os::readLine() didn't cleared the line after "
                "finishing reading\n");

        line = "initial line content for an extra os::readLine()";
        if (dpso::os::readLine(fp.get(), line))
            test::failure(
                "An extra os::readLine() returned true after reading "
                "was finished\n");

        if (!line.empty())
            test::failure(
                "An extra os::readLine() didn't cleared the line\n");

        if (std::ferror(fp.get()))
            test::fatalError(
                "Error while reading \"%s\"\n", fileName);

        if (lines != test.expectedLines)
            test::failure(
                "Unexpected lines from os::readLine() if \"%s\": "
                "expected %s, got %s\n",
                test::utils::escapeStr(test.text).c_str(),
                test::utils::toStr(
                    test.expectedLines.begin(),
                    test.expectedLines.end()).c_str(),
                test::utils::toStr(
                    lines.begin(), lines.end()).c_str());
    }

    test::utils::removeFile(fileName);
}


void testRemoveFile()
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

    try {
        dpso::os::removeFile("nonexistent_file");
        test::failure(
            "os::removeFile() for a nonexistent file didn't threw an "
            "error\n");
    } catch (dpso::os::FileNotFoundError&) {
    } catch (dpso::os::Error& e) {
        test::failure(
            "os::removeFile() for a nonexistent file threw an error "
            "(\"%s\") of class other than FileNotFoundError\n",
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
            "os::replace(\"%s\", \"%s\"): %s\n",
            srcFilePath, dstFilePath, e.what());
        return;
    }

    if (dpso::os::StdFileUPtr{dpso::os::fopen(srcFilePath, "r")})
        test::failure(
            "os::replace(\"%s\", \"%s\") didn't failed, but the "
            "source file still exists\n",
            srcFilePath, dstFilePath);

    if (!dstExists
            && !dpso::os::StdFileUPtr{
                dpso::os::fopen(dstFilePath, "r")}) {
        test::failure(
            "os::replace(\"%s\", \"%s\") didn't created the "
            "destination file\n",
            srcFilePath, dstFilePath);
        return;
    }

    const auto dstText = test::utils::loadText(
        "testReplace", dstFilePath);

    if (dstText != srcFilePath)
        test::failure(
            "os::replace(\"%s\", \"%s\"): the destination file "
            "contents (\"%s\") are not the same as in source "
            "(\"%s\")\n",
            srcFilePath, dstFilePath, dstText.c_str(), srcFilePath);
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
            "(\"%s\") of class other than FileNotFoundError\n",
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


void testOs()
{
    testPathSplit();
    testGetFileExt();
    testGetFileSize();
    testFopen();
    testReadLine();
    testRemoveFile();
    testReplace();
    testSyncFile();
    testSyncFileDir();
}


}


REGISTER_TEST(testOs);
