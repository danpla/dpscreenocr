
#include "unix/make_dirs.h"

#include <string>

#include <sys/stat.h>


namespace dpso::unix {


bool makeDirs(char* path, mode_t mode)
{
    auto* s = path;

    // Root always exists.
    while (*s == '/')
        ++s;

    while (*s) {
        while (*s && *s != '/')
            ++s;

        while (*s == '/')
            ++s;

        const auto c = *s;
        *s = 0;
        // Force 0777 mode for intermediate directories so that mkdir
        // can create a directory with read or write permissions
        // removed when the same permissions are used for a newly
        // created parent directory.
        const auto ret = mkdir(path, *s ? 0777 : mode);
        *s = c;

        if (ret != 0 && errno != EEXIST)
            return false;
    }

    return true;
}


bool makeDirs(const char* path, mode_t mode)
{
    std::string pathCopy{path};
    return makeDirs(pathCopy.data(), mode);
}


}
