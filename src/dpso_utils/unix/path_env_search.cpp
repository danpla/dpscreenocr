
#include "unix/path_env_search.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>


namespace dpso::unix {


std::string findInPathEnv(const char* name)
{
    if (strchr(name, '/'))
        return {};

    const auto* p = getenv("PATH");
    if (!p || !*p)
        return {};

    std::string path;
    while (true) {
        const auto* pathBegin = p;
        while (*p && *p != ':')
            ++p;
        const auto* pathEnd = p;

        path.assign(pathBegin, pathEnd);

        // An empty path is a legacy feature indicating the current
        // working directory. It can appear as two colons in the
        // middle of the list, as well a colon at the beginning or end
        // of the list.
        if (!path.empty() && path.back() != '/')
            path += '/';

        path += name;

        if (access(path.c_str(), X_OK) == 0)
            return path;

        if (!*p)
            break;

        ++p;
    }

    return {};
}


}
