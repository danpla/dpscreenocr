
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
        // Force 0777 mode for intermediate directories to emulate
        // `mkdirs -p` behavior. Without this, mkdir() will not be
        // able to create a directory with write or read permissions
        // removed, because the same permissions will be used for the
        // newly created parent dir.
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
