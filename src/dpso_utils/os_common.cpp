
#include "os.h"

#include <chrono>
#include <cstring>
#include <thread>


void dpsoSleep(int milliseconds)
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds{milliseconds});
}


const char* dpsoGetFileExt(const char* filePath)
{
    const char* ext{};

    for (const auto* s = filePath; *s; ++s)
        if (*s == '.')
            ext = s;
        else if (std::strchr(dpsoDirSeparators, *s))
            ext = nullptr;

    if (ext
            && ext[1]
            // A leading period denotes a "hidden" file on Unix-like
            // systems. We follow this convention on all platforms.
            && ext != filePath
            && !std::strchr(dpsoDirSeparators, ext[-1]))
        return ext;

    return nullptr;
}
