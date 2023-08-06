
#include "os.h"

#include <cstring>


namespace dpso::os {


const char* getFileExt(const char* filePath)
{
    const char* ext{};

    for (const auto* s = filePath; *s; ++s)
        if (*s == '.')
            ext = s;
        else if (std::strchr(dirSeparators, *s))
            ext = nullptr;

    if (ext
            && ext[1]
            // A leading period denotes a "hidden" file on Unix-like
            // systems. We follow this convention on all platforms.
            && ext != filePath
            && !std::strchr(dirSeparators, ext[-1]))
        return ext;

    return nullptr;
}


}
