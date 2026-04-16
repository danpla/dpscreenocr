#include "unix/exe_path.h"

#include <errno.h>
#include <stdlib.h>

#include "os_error.h"
#include "str.h"
#include "unix/path_env_search.h"


namespace dpso::unix {


std::string getExePath(std::string_view name)
{
    std::string path;

    if (name.find('/') != name.npos)
        path = name;
    else {
        path = findInPathEnv(name);
        if (path.empty())
            throw os::Error{str::format(
                "Can't find \"{}\" in $PATH", name)};
    }

    auto* realPath = realpath(path.c_str(), nullptr);
    if (!realPath)
        throw os::Error{str::format(
            "realpath(\"{}\"): {}", path, os::getErrnoMsg(errno))};

    std::string result{realPath};
    free(realPath);

    return result;
}


}
