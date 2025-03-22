#include "init_extra.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


namespace ui {


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


bool initStart(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    return true;
}


bool initEnd(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    registerApplicationRestart();
    return true;
}


}
