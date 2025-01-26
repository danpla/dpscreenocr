#include "unix/exe_path.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "os.h"
#include "str.h"
#include "unix/path_env_search.h"


namespace dpso::unix {


std::string getExePath(const char* name)
{
    std::string path;

    if (strchr(name, '/'))
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

    std::string result = realPath;
    free(realPath);

    return result;
}


}
