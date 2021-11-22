
#include "cfg_path.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>

#include "dpso/error.h"
#include "os.h"
#include "unix_utils.h"


// https://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
const char* dpsoGetCfgPath(const char* appName)
{
    static std::string path;
    path.clear();

    const auto* xdgConfigHome = std::getenv("XDG_CONFIG_HOME");
    // According to the specification, we should treat empty
    // XDG_CONFIG_HOME as unset.
    if (xdgConfigHome && *xdgConfigHome)
        path += xdgConfigHome;
    else if (const auto* home = std::getenv("HOME")) {
        path += home;
        path += "/.config";
    }
    else {
        dpsoSetError(
            "Neither XDG_CONFIG_HOME nor HOME environment variable "
            "is set");
        return nullptr;
    }

    path += '/';
    path += appName;

    if (!dpso::unix::makeDirs(&path[0])) {
        dpsoSetError((
            "makeDirs(\"" + path + "\") failed: "
            + std::strerror(errno)).c_str());
        return nullptr;
    }

    return path.c_str();
}
