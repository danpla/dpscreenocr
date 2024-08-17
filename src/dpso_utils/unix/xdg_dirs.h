
#pragma once

#include <string>


namespace dpso::unix {


enum class XdgDir {
    dataHome,
    configHome
};


const char* toStr(XdgDir dir);


// Return an absolute path of a directory defined by XDG Base
// Directory Specification:
// https://specifications.freedesktop.org/basedir-spec/latest/
// Throws os::Error if neither a corresponding XDG_* nor HOME
// environment variable is set.
std::string getXdgDirPath(XdgDir dir);


}
