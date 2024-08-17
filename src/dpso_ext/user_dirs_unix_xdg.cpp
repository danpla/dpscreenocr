
#include "user_dirs.h"

#include <cassert>
#include <string>

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"
#include "dpso_utils/unix/xdg_dirs.h"


using namespace dpso;


static unix::XdgDir getXdgDir(DpsoUserDir userDir)
{
    switch (userDir) {
    case DpsoUserDirConfig:
        return unix::XdgDir::configHome;
    case DpsoUserDirData:
        return unix::XdgDir::dataHome;
    }

    assert(false);
    return {};
}


const char* dpsoGetUserDir(DpsoUserDir userDir, const char* appName)
{
    static std::string path;

    const auto xdgDir = getXdgDir(userDir);

    try {
        path = unix::getXdgDirPath(xdgDir);
    } catch (os::Error& e) {
        setError("unix::getXdgDirPath({}): {}", xdgDir, e.what());
        return {};
    }

    path += '/';
    path += appName;

    try {
        os::makeDirs(path.c_str());
    } catch (os::Error& e) {
        setError("os::makeDirs(\"{}\"): {}", path, e.what());
        return {};
    }

    return path.c_str();
}
