
#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <stdexcept>


namespace dpso::os {


class Error : public std::runtime_error {
    using runtime_error::runtime_error;
};


class FileNotFoundError : public Error {
    using Error::Error;
};


// Directory separators for the current platform. The primary one is
// the first in the list.
extern const char* const dirSeparators;


// Return a pointer to the period of the extension, or
// null if filePath has no extension.
const char* getFileExt(const char* filePath);



// Return size of filePath in bytes. The behavior is platform-specific
// if the filePath points to anything other than a regular file.
// Throws os::Error.
std::int64_t getFileSize(const char* filePath);


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


// Throws os::Error.
void removeFile(const char* filePath);


// Rename a file or directory, replacing destination.
//
// Unlike std::rename(), this function silently replaces an existing
// dst on all platforms.
//
// Throws os::Error.
void replace(const char* src, const char* dst);


// Create a chain of directories.
//
// Throws os::Error. An existing dirPath is not treated as an error.
void makeDirs(const char* dirPath);


// Synchronize file state with storage device.
//
// The function transfers all modified data (and possibly file
// attributes) to the permanent storage device. It should normally be
// preceded by fflush().
//
// Throws os::Error.
void syncFile(std::FILE* fp);


// Synchronize directory containing filePath with storage device.
//
// The function is an equivalent of syncFile() for directories: it's
// usually called after creating a file to ensure that the new
// directory entry has reached the storage device.
//
// Throws os::Error.
void syncFileDir(const char* filePath);


}
