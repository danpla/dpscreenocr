
#include "unix/xdg_dirs.h"

#include <cassert>
#include <cstdlib>

#include "os.h"
#include "str.h"


namespace dpso::unix {


const char* toStr(XdgDir dir)
{
    #define CASE(DIR) case DIR: return #DIR

    switch (dir) {
    CASE(XdgDir::dataHome);
    CASE(XdgDir::configHome);
    }

    #undef CASE

    assert(false);
    return "";
}


static std::string getDir(
    const char* xdgHomeEnv, const char* homeFallbackDir)
{
    std::string result;

    if (const auto* xdgHome = std::getenv(xdgHomeEnv);
            // According to the specification, a XDG_*_HOME variable:
            // * Should be treated as unset if empty
            // * Invalid if contains a relative path
            xdgHome && *xdgHome == '/')
        result += xdgHome;
    else if (const auto* home = std::getenv("HOME")) {
        result += home;
        result += '/';
        result += homeFallbackDir;
    } else
        throw os::Error{str::format(
            "Neither {} nor HOME environment variable is set",
            xdgHomeEnv)};

    return result;
}


std::string getXdgDirPath(XdgDir dir)
{
    switch (dir) {
    case XdgDir::dataHome:
        return getDir("XDG_DATA_HOME", ".local/share");
    case XdgDir::configHome:
        return getDir("XDG_CONFIG_HOME", ".config");
    }

    assert(false);
    return {};
}


}