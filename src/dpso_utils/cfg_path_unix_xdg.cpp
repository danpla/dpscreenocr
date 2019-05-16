
#include "cfg_path.h"

#include <cerrno>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#include "os.h"


static bool makeDirs(char* path, mode_t mode)
{
    for (auto* s = &path[1]; *s; ++s) {
        if (*s != '/')
            continue;

        *s = 0;
        const auto ret = mkdir(path, mode);
        *s = '/';

        if (ret != 0 && errno != EEXIST)
            return false;
    }

    return mkdir(path, mode) == 0 || errno == EEXIST;
}


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

    makeDirs(&path[0], 0777);

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
