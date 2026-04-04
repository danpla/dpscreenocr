#include "toplevel_argv0.h"

#include <cstdlib>


namespace ui {


std::string_view getToplevelArgv0(std::string_view argv0)
{
    const auto* launcherArgv0 = std::getenv("LAUNCHER_ARGV0");
    return launcherArgv0 && *launcherArgv0 ? launcherArgv0 : argv0;
}


}
