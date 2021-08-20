
#include <algorithm>
#include <cstdio>
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
        std::fprintf(
            stderr,
            "utf8ToUtf16(\"%s\") failed: %s\n",
            test::utils::escapeStr(utf8Str).c_str(), e.what());
        test::failure();
    }

    std::string utf8ToUtf16Result;
    try {
        utf8ToUtf16Result = utf16ToUtf8(utf16Str);
    } catch (std::runtime_error& e) {
        std::fprintf(
            stderr,
            "utf16ToUtf8() failed to convert result of "
            "utf8ToUtf16(\"%s\"): %s\n",
            test::utils::escapeStr(utf8Str).c_str(), e.what());
        test::failure();
    }

    if (utf8ToUtf16Result != utf8Str) {
        std::fprintf(
            stderr,
            "utf8ToUtf16() <=> utf16ToUtf8() conversion failed.\n"
            "  Original:  %s\n"
            "  Converted: %s\n",
            test::utils::escapeStr(utf8Str).c_str(),
            test::utils::escapeStr(
                utf8ToUtf16Result.c_str()).c_str());
        test::failure();
    }
}


static void testUtfConversions()
{
    const char* const utf8Strings[] = {
        // Юникод
        "\320\256\320\275\320\270\320\272\320\276\320\264"
    };

    for (const auto* utf8Str : utf8Strings)
        testUtfConversion(utf8Str);
}


// Wrapper around CommandLineToArgvW()
static std::vector<std::string> cmdLineToArgv(
    const std::string& cmdLine)
{
    int argc;
    wchar_t** argv = CommandLineToArgvW(
        dpso::windows::utf8ToUtf16(cmdLine).c_str(), &argc);

    if (!argv) {
        std::fprintf(
            stderr,
            "CommandLineToArgvW(%s) failed with error %lu.\n",
            cmdLine.c_str(), GetLastError());
        return {};
    }

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

    const auto gotArgv = cmdLineToArgv(cmdLine);

    if (gotArgv.size() != argv.size()) {
        std::fprintf(
            stderr,
            "CommandLineTogotArgvW(%s) returned a different number "
            "of arguments: %zu (was %zu in original array). "
            "Returned array:\n"
            "  %s\n",
            cmdLine.c_str(), gotArgv.size(), argv.size(),
            rangeToString(gotArgv.begin(), gotArgv.end()).c_str());
        test::failure();
        return;
    }

    if (!std::equal(gotArgv.begin(), gotArgv.end(), argv.begin())) {
        std::fprintf(
            stderr,
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
        test::failure();
    }
}


static void testCreateCmdLine()
{
    const char* programName = "program name";
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
    testCreateCmdLine();
}


REGISTER_TEST("windows_utils", testWindowsUtils);
