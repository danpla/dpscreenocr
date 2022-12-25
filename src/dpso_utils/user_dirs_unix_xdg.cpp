
#include "user_dirs.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>

#include "dpso/error.h"
#include "unix_utils.h"


// https://specifications.freedesktop.org/basedir-spec/latest/
static const char* getDir(
    const char* xdgHomeEnv,
    const char* homeFallbackDir,
    const char* appName)
{
    static std::string path;
    path.clear();

    if (const auto* xdgHome = std::getenv(xdgHomeEnv);
            // According to the specification, we should treat empty
            // XDG_*_HOME as unset.
            xdgHome && *xdgHome)
        path += xdgHome;
    else if (const auto* home = std::getenv("HOME")) {
        path += home;
        path += '/';
        path += homeFallbackDir;
    } else {
        dpsoSetError(
            "Neither %s nor HOME environment variable is set",
            xdgHomeEnv);
        return nullptr;
    }

    path += '/';
    path += appName;

    if (!dpso::unix::makeDirs(&path[0])) {
        dpsoSetError(
            "makeDirs(\"%s\"): %s",
            path.c_str(), std::strerror(errno));
        return nullptr;
    }

    return path.c_str();
}


const char* dpsoGetUserDir(DpsoUserDir userDir, const char* appName)
{
    switch (userDir) {
    case DpsoUserDirConfig:
        return getDir("XDG_CONFIG_HOME", ".config", appName);
    case DpsoUserDirData:
        return getDir("XDG_DATA_HOME", ".local/share", appName);
    }

    return {};
}
