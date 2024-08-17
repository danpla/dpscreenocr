
#include "toplevel_argv0.h"

#include <cstdlib>


namespace ui {


const char* getToplevelArgv0(const char* argv0)
{
    const auto* launcherArgv0 = std::getenv("LAUNCHER_ARGV0");
    return launcherArgv0 && *launcherArgv0 ? launcherArgv0 : argv0;
}


}
