#include "init_extra.h"

#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "msix_helper/msix_helper.h"


namespace ui {


// When the executable is compiled with the "windows" target subsystem
// (/subsystem:windows in MSVC or -mwindows in GCC), the Windows
// execution environment will not set up the standard handles for the
// console IO, so the standard C streams will not work as well (e.g.,
// you will not see any output from printf() in cmd.exe).
//
// The "windows" is the standard subsystem for GUI apps, so any GUI
// app that provides a command-line interface must connect to the
// console manually and then update any objects (e.g. FILE*s of the
// standard C streams) that use STD_*_HANDLE handles.
static void attachConsole()
{
    // Since AttachConsole() unconditionally points all handles to the
    // console, we must not reopen streams for handles that were valid
    // (e.g. redirected to a file) before the call.
    const auto isValid = [](DWORD stdHandle)
    {
        const auto handle = GetStdHandle(stdHandle);
        return handle && handle != INVALID_HANDLE_VALUE;
    };

    const auto stdoutValid = isValid(STD_OUTPUT_HANDLE);
    const auto stderrValid = isValid(STD_ERROR_HANDLE);
    // We don't care about stdin since our app does not use it.

    if (stdoutValid && stderrValid)
        return;

    if (!AttachConsole(ATTACH_PARENT_PROCESS))
        return;

    const auto reopen = [](DWORD stdHandle, std::FILE* stream)
    {
        const auto handle = GetStdHandle(stdHandle);
        if (handle && handle != INVALID_HANDLE_VALUE)
            (void)std::freopen("CONOUT$", "w", stream);
    };

    if (!stdoutValid)
        reopen(STD_OUTPUT_HANDLE, stdout);

    if (!stderrValid)
        reopen(STD_ERROR_HANDLE, stderr);
}


// The main goal of registering restart is to give the installer
// (e.g. Inno Setup) an ability to restart our application in case it
// was automatically closed before installing an update.
static void registerApplicationRestart()
{
    const auto* cmdLine = GetCommandLineW();
    // The command line is actually never empty, but check anyway for
    // the code below.
    if (!*cmdLine)
        return;

    // RegisterApplicationRestart() doesn't need the path to the
    // executable, so skip it. It may be in double quotes if it
    // contains spaces.
    const auto endChar = *cmdLine++ == L'\"' ? L'\"' : L' ';
    while (*cmdLine)
        if (*cmdLine++ == endChar)
            break;

    while (*cmdLine == L' ')
        ++cmdLine;

    RegisterApplicationRestart(
        cmdLine, RESTART_NO_CRASH | RESTART_NO_HANG);
}


bool initStart(int /*argc*/, char* /*argv*/[])
{
    attachConsole();
    return true;
}


bool initEnd(UiStartupArgs& startupArgs)
{
    if (msix::isActivatedByStartupTask())
        startupArgs.hide = true;

    registerApplicationRestart();
    return true;
}


}
