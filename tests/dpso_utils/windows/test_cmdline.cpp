
#include <algorithm>
#include <initializer_list>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "dpso_utils/windows/cmdline.h"
#include "dpso_utils/windows/utf.h"

#include "flow.h"
#include "utils.h"


namespace {


// Wrapper around CommandLineToArgvW()
std::vector<std::string> cmdLineToArgv(const char* cmdLine)
{
    int argc;
    auto** argv = CommandLineToArgvW(
        dpso::windows::utf8ToUtf16(cmdLine).c_str(), &argc);

    if (!argv)
        test::fatalError(
            "CommandLineToArgvW({}): {}", cmdLine, GetLastError());

    std::vector<std::string> result;
    result.reserve(argc);

    for (int i = 0; i < argc; ++i)
        result.push_back(dpso::windows::utf16ToUtf8(argv[i]));

    LocalFree(argv);
    return result;
}


void testArgv(std::initializer_list<const char*> argv)
{
    using namespace dpso::windows;

    const auto cmdLine = createCmdLine(
        *argv.begin(), argv.begin() + 1, argv.size() - 1);

    const auto gotArgv = cmdLineToArgv(cmdLine.c_str());

    if (gotArgv.size() != argv.size()) {
        test::failure(
            "CommandLineTogotArgvW({}) returned a different number "
            "of arguments: {} (was {} in original array). "
            "Returned array:\n"
            "  {}",
            cmdLine,
            gotArgv.size(),
            argv.size(),
            test::utils::toStr(gotArgv));
        return;
    }

    if (!std::equal(gotArgv.begin(), gotArgv.end(), argv.begin()))
        test::failure(
            "createCmdLine() and CommandLineTogotArgvW() don't "
            "match.\n"
            "  createCmdLine() string:\n"
            "    {}\n"
            "  Original argv passed to createCmdLine():\n"
            "    {}\n"
            "  CommandLineTogotArgvW() from createCmdLine() string:\n"
            "    {}",
            cmdLine,
            test::utils::toStr(argv),
            test::utils::toStr(gotArgv));
}


void testWindowsCmdLine()
{
    const auto* programName = "program name";
    const std::initializer_list<const char*> argvs[]{
        {programName, "a\\\\b", "de fg", "h"},
        {programName, "a\\\"b", "c", "d"},
        {programName, "a\\\\b c", "d", "e"},
        {programName, "", "a"},
    };

    for (const auto& argv : argvs)
        testArgv(argv);
}


}


REGISTER_TEST(testWindowsCmdLine);
