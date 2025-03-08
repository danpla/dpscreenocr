#pragma once

#include <cstdio>


namespace dpso::os {


// fopen() that accepts filePath in UTF-8.
std::FILE* fopen(const char* filePath, const char* mode);


// Deleter for a FILE* smart pointer.
struct StdFileCloser {
    void operator()(std::FILE* fp) const
    {
        if (fp)
            std::fclose(fp);
    }
};


using StdFileUPtr = std::unique_ptr<std::FILE, StdFileCloser>;


// Synchronize file state with storage device.
//
// The function transfers all modified data (and possibly file
// attributes) to the permanent storage device. It should normally be
// preceded by fflush().
//
// Throws os::Error.
void syncFile(std::FILE* fp);


}
