
#include <algorithm>
#include <initializer_list>
#include <vector>

#include <windows.h>

#include "dpso_utils/windows_utils.h"

#include "flow.h"
#include "utils.h"


static void testUtfConversion(const char* utf8Str)
{
    using namespace dpso::windows;

    std::wstring utf16Str;

    try {
        utf16Str = utf8ToUtf16(utf8Str);
    } catch (std::runtime_error& e) {
        test::failure(
            "utf8ToUtf16(\"%s\"): %s\n",
            test::utils::escapeStr(utf8Str).c_str(), e.what());
        return;
    }

    std::string utf16ToUtf8Result;
    try {
        utf16ToUtf8Result = utf16ToUtf8(utf16Str.c_str());
    } catch (std::runtime_error& e) {
        test::failure(
            "utf16ToUtf8() failed to convert result of "
            "utf8ToUtf16(\"%s\"): %s\n",
            test::utils::escapeStr(utf8Str).c_str(), e.what());
        return;
    }

    if (utf16ToUtf8Result != utf8Str)
        test::failure(
            "utf8ToUtf16() <=> utf16ToUtf8() conversion failed.\n"
            "  Original:  %s\n"
            "  Converted: %s\n",
            test::utils::escapeStr(utf8Str).c_str(),
            test::utils::escapeStr(
                utf16ToUtf8Result.c_str()).c_str());
}


static void testUtfConversions()
{
    const char* const utf8Strings[] = {
        "",
        // Юникод
        "\320\256\320\275\320\270\320\272\320\276\320\264"
    };

    for (const auto* utf8Str : utf8Strings)
        testUtfConversion(utf8Str);
}


static void testInvalidUtf8()
{
    using namespace dpso::windows;

    const char* const strings[] = {
        // Invalid start byte 0x80 - 0xbf
        "\x80",
        "\xbf",

        // Surrogates 0xd800 - 0xdfff
        "\xd8\x00",
        "\xdf\xff",

        // Overlong 2 bytes (\xC0\x80 - \xC1\xBF)
        "\xc0\x80",
        "\xc1\xbf",

        // Overlong 3 bytes (\xE0\x80\x80 - \xE0\x9F\xBF)
        "\xe0\x80\x80",
        "\xe0\x9f\xbf",

        // Overlong 4 bytes (\xF0\x80\x80\x80 - \xF0\x8F\xBF\xBF)
        "\xf0\x80\x80\x80",
        "\xf0\x8f\xbf\xbf",

        // Out of bounds (> 0x10ffff)
        "\xf4\x90\x80\x80",
    };

    for (const auto* s : strings) {
        try {
            dpso::windows::utf8ToUtf16(s);
            test::failure(
                "utf8ToUtf16(\"%s\") doesn't throw\n",
                test::utils::escapeStr(s).c_str());
        } catch (std::runtime_error& e) {
        }
    }
}


static void testInvalidUtf16()
{
    const wchar_t* const strings[] = {
        L"\xd800",  // min leading
        L"\xdbff",  // max leading
        L"\xdc00",  // min trailing
        L"\xdfff",  // max trailing
        L"\xd800\xdbff",  // min leading + (min trailing - 1)
        L"\xd800\xe000",  // min leading + (max trailing + 1)
        L"\xdbff\xdbff",  // max leading + (min trailing - 1)
        L"\xdbff\xe000",  // max leading + (max trailing + 1)
    };

    for (const auto* s : strings) {
        try {
            dpso::windows::utf16ToUtf8(s);
            test::failure(
                "utf16ToUtf8() doesn't throw on invalid UTF-16\n");
        } catch (std::runtime_error& e) {
        }
    }
}


// Wrapper around CommandLineToArgvW()
static std::vector<std::string> cmdLineToArgv(const char* cmdLine)
{
    int argc;
    auto** argv = CommandLineToArgvW(
        dpso::windows::utf8ToUtf16(cmdLine).c_str(), &argc);

    if (!argv)
        test::fatalError(
            "CommandLineToArgvW(%s) failed with error %lu.\n",
            cmdLine, GetLastError());

    std::vector<std::string> result;
    result.reserve(argc);

    for (int i = 0; i < argc; ++i)
        result.push_back(dpso::windows::utf16ToUtf8(argv[i]));

    LocalFree(argv);
    return result;
}


template<typename T>
std::string rangeToString(T begin, T end)
{
    std::string result = "[";

    for (; begin != end; ++begin) {
        if (result.size() > 1)
            result += ", ";

        result += *begin;
    }

    result += ']';
    return result;
}


static void testArgv(std::initializer_list<const char*> argv)
{
    using namespace dpso::windows;

    const auto cmdLine = createCmdLine(
        *argv.begin(), argv.begin() + 1, argv.size() - 1);

    const auto gotArgv = cmdLineToArgv(cmdLine.c_str());

    if (gotArgv.size() != argv.size()) {
        test::failure(
            "CommandLineTogotArgvW(%s) returned a different number "
            "of arguments: %zu (was %zu in original array). "
            "Returned array:\n"
            "  %s\n",
            cmdLine.c_str(), gotArgv.size(), argv.size(),
            rangeToString(gotArgv.begin(), gotArgv.end()).c_str());
        return;
    }

    if (!std::equal(gotArgv.begin(), gotArgv.end(), argv.begin()))
        test::failure(
            "createCmdLine() and CommandLineTogotArgvW() don't "
            "match.\n"
            "  createCmdLine() string:\n"
            "    %s\n"
            "  Original argv passed to createCmdLine():\n"
            "    %s\n"
            "  CommandLineTogotArgvW() from createCmdLine() string:\n"
            "    %s\n",
            cmdLine.c_str(),
            rangeToString(argv.begin(), argv.end()).c_str(),
            rangeToString(gotArgv.begin(), gotArgv.end()).c_str());
}


static void testCreateCmdLine()
{
    const auto* programName = "program name";
    const std::initializer_list<const char*> argvs[] = {
        {programName, "a\\\\b", "de fg", "h"},
        {programName, "a\\\"b", "c", "d"},
        {programName, "a\\\\b c", "d", "e"},
    };

    for (const auto& argv : argvs)
        testArgv(argv);
}


static void testWindowsUtils()
{
    testUtfConversions();
    testInvalidUtf8();
    testInvalidUtf16();
    testCreateCmdLine();
}


REGISTER_TEST(testWindowsUtils);
