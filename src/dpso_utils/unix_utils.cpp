
#include "unix_utils.h"

#include <cerrno>
#include <string>


namespace dpso {
namespace unix {


bool makeDirs(char* path, mode_t mode)
{
    auto* s = path;

    // Root always exists.
    while (*s == '/')
        ++s;

    while (true) {
        // Skip till the first separator.
        while (*s && *s != '/')
            ++s;

        // Consume consecutive separators.
        while (*s && s[1] == '/')
            ++s;

        if (!*s || !s[1])
            // This is the last directory.
            break;

        *s = 0;
        // Force 0777 mode for intermediate directories to emulate
        // `mkdirs -p` behavior. Without this, mkdir() will not be
        // able to create a directory with write or read permissions
        // removed, because the same permissions will be used for the
        // newly created parent dir.
        const auto ret = mkdir(path, 0777);
        *s = '/';

        if (ret != 0 && errno != EEXIST)
            return false;

        ++s;
    }

    return mkdir(path, mode) == 0 || errno == EEXIST;
}


bool makeDirs(const char* path, mode_t mode)
{
    std::string pathCopy = path;
    return makeDirs(&pathCopy[0], mode);
}


}
}
