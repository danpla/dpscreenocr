
#include "exec.h"

#include <cctype>
#include <initializer_list>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "dpso/backend/windows/utils/com.h"
#include "dpso/str.h"
#include "os.h"
#include "windows_utils.h"


// It seems that it's just impossible to securely pass random text as
// a parameter to a batch file.
//
// Although the article [1] suggest to use ^ escape for every
// metacharacter, this only works for one level of cmd.exe invocation,
// e.g., when we invoke a regular binary executable via cmd.exe. For
// example, let's take the following string:
//
//    arg"&whoami
//
// To pass it to the normal executable (not a batch file) along with
// other arguments, we should transform it to the following:
//
//    "arg\"&whoami"
//
// That is, we escape the internal double quote and put double quotes
// around the argument. Now let's do what the article suggest: escape
// all metacharacters, including double quotes:
//
//    ^"arg\^"^&whoami^"
//
// This doesn't work because we have two levels of unescaping: first
// in the parameters of cmd.exe that invokes the batch file, then in
// the batch file itself. If we pass the argument like this, the batch
// file will actually receive the original "arg\"&whoami". One may
// think that adding two levels of escapes will solve the issue:
//
//    ^^^"arg\^^^"^^^&whoami^^^"
//
// Indeed this may work if the batch will only access the argument
// directly like:
//
//    echo %1
//
// But if we change it to the following:
//
//    set p=%1
//    echo %p%
//
// Everything is broken again since the argument is unescaped twice:
// first when it's assigned to the variable, and then when that
// variable is used. The similar issue happen when a batch file calls
// another one. We now need 3 levels of escapes:
//
//    ^^^^^^^"arg\^^^^^^^"^^^^^^^&whoami^^^^^^^"
//
// This is just ridiculous.
//
// And the main question: what to do if a batch file calls both
// another batch file and a normal executable?
//
// Obviously, we can't detect any of the cases mentioned. Even if we
// add two levels of ^-escapes for an executable that have a ".bat"
// extension, we can't assume that this batch doesn't call another
// one or doesn't use variables.
//
// Maybe I missed something and everything written above is completely
// wrong, but for now it seems like batch files are broken by design
// and therefore we just can't allow users to execute them.
//
// Note: The examples above can be tested:
//
//   * By typing them directly in cmd.exe
//
//   * By using them with lpCommandLine argument of CreateProcess(),
//     with or without "cmd.exe /c" prefix like:
//
//       * "cmd.exe /c batch.bat ^"arg\^"^&whoami^""
//       * "batch.bat ^"arg\^"^&whoami^""
//
//     In the last case, cmd.exe is called implicitly (at least on
//     Windows 7).
//
//   * By using ShellExecute, which acts like a wrapper around
//     CreateProcess() in this particular case. Two ways:
//
//       * lpFile: path to BAT; lpParameters: "^"arg\^"^&whoami^""
//       * lpFile: "cmd"; lpParameters: "/c ^"arg\^"^&whoami^""
//
// [1] https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/


static bool allowExecute(const char* exePath)
{
    // Don't execute an empty string since ShellExecute() opens
    // current working directory in Explorer in this case.
    while (std::isspace(*exePath))
        ++exePath;
    if (!*exePath)
        return false;

    const auto* ext = dpsoGetFileExt(exePath);
    if (!ext)
        return true;

    // Don't execute batch scripts, taking into account the fact that
    // the string can contain trailing whitespace that will be
    // stripped by ShellExecute().
    const auto* extEnd = ext;
    for (const auto* s = ext; *s; ++s)
        if (!std::isspace(*s))
            extEnd = s + 1;

    for (const auto* batchExt : {".bat", ".cmd"})
        if (dpso::str::cmpSubStr(
                batchExt,
                ext,
                extEnd - ext,
                dpso::str::cmpIgnoreCase) == 0)
            return false;

    return true;
}


void dpsoExec(const char* exePath, const char* arg)
{
    if (!allowExecute(exePath))
        return;

    dpso::windows::CoInitializer coInitializer{
        COINIT_APARTMENTTHREADED};

    const auto cmdLine = dpso::windows::createCmdLine("", {arg});

    std::wstring exePathUtf16;
    std::wstring cmdLineUtf16;
    try {
        exePathUtf16 = dpso::windows::utf8ToUtf16(exePath);
        cmdLineUtf16 = dpso::windows::utf8ToUtf16(cmdLine.c_str());
    } catch (std::runtime_error&) {
        return;
    }

    // We use ShellExecute(), as it allows to launch a script (like
    // Python) directly without the need to invoke the interpreter
    // explicitly, that is, it does the same as double-click on the
    // script file in Explorer. To do that with CreateProcess(), we
    // need to pass the script path trough cmd.exe, which requires
    // escaping of metacharacters with ^, and a workaround for
    // newlines in arguments.
    //
    // For a high-level info about ShellExecute(), see:
    //   https://docs.microsoft.com/en-us/windows/desktop/shell/launch
    SHELLEXECUTEINFOW si{};
    si.cbSize = sizeof(si);
    si.fMask = SEE_MASK_NOCLOSEPROCESS;
    si.lpFile = exePathUtf16.c_str();
    si.lpParameters = cmdLineUtf16.c_str();
    si.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&si))
        return;

    WaitForSingleObject(si.hProcess, INFINITE);
    CloseHandle(si.hProcess);
}
