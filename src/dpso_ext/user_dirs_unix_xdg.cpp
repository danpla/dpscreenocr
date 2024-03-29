
#include "user_dirs.h"

#include <cstdlib>
#include <string>

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"


// https://specifications.freedesktop.org/basedir-spec/latest/
static const char* getDir(
    const char* xdgHomeEnv,
    const char* homeFallbackDir,
    const char* appName)
{
    static std::string path;
    path.clear();

    if (const auto* xdgHome = std::getenv(xdgHomeEnv);
            // According to the specification, a XDG_*_HOME variable:
            // * Should be treated as unset if empty
            // * Invalid if contains a relative path
            xdgHome && *xdgHome == '/')
        path += xdgHome;
    else if (const auto* home = std::getenv("HOME")) {
        path += home;
        path += '/';
        path += homeFallbackDir;
    } else {
        dpso::setError(
            "Neither {} nor HOME environment variable is set",
            xdgHomeEnv);
        return nullptr;
    }

    path += '/';
    path += appName;

    try {
        dpso::os::makeDirs(path.c_str());
    } catch (dpso::os::Error& e) {
        dpso::setError("os::makeDirs(\"{}\"): {}", path, e.what());
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
