
#include "cfg_path.h"

#include <cerrno>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#include "os.h"
#include "unix_utils.h"


// https://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
const char* dpsoGetCfgPath(const char* appName)
{
    static std::string path;
    path.clear();

    auto* xdgConfigHome = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfigHome && *xdgConfigHome)
        path += xdgConfigHome;
    else if (auto* home = std::getenv("HOME")) {
        path += home;
        path += "/.config";
    }

    // If even $HOME is not set, we will try to create appName
    // dir in the current working directory.
    if (!path.empty())
        path += '/';

    path += appName;

    dpso::unix::makeDirs(&path[0]);

    return path.c_str();
}


FILE* dpsoCfgPathFopen(
    const char* appName, const char* fileName, const char* mode)
{
    std::string path = dpsoGetCfgPath(appName);
    path += '/';
    path += fileName;
    return dpso::fopenUtf8(path.c_str(), mode);
}
