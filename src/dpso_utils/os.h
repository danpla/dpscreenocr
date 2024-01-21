
#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>


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


// Return the path without the last component. The result will not
// include trailing separators, unless it's a root directory. Returns
// an empty string if the path does not contain a directory (e.g., if
// it doesn't have separators).
std::string getDirName(const char* path);


// Return the last path component, or an empty string if there is
// none.
std::string getBaseName(const char* path);


// Convert path from UTF-8 to the system-specific encoding.
//
// This function is intended to be used with third-party libraries
// that don't handle UTF-8 paths on Windows. It's only necessary for
// compatibility with Windows versions older than 10 build 1903, as
// they don't support enabling UTF-8 via application manifests.
//
// On systems other than Windows, the function returns the path as is.
//
// Throws os::Error.
std::string convertUtf8PathToSys(const char* utf8Path);


// Return a pointer to the period of the extension, or null if
// filePath has no extension.
const char* getFileExt(const char* filePath);


// Return size of filePath in bytes. The behavior is platform-specific
// if the filePath points to anything other than a regular file.
// Throws os::Error.
std::int64_t getFileSize(const char* filePath);


// Return the description of the given errno number. This is similar
// to std::strerror(), except that it's locale-independent and
// thread-safe, if supported by the platform. If these features are
// not supported by the platform, then this function may just be a
// wrapper for std::strerror().
std::string getErrnoMsg(int errnum);


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


// Read up to dstSize bytes from a file. Returns the number of bytes
// read, which may be less than dstSize if the end of the file is
// reached.
//
// Throws os::Error.
std::size_t readSome(std::FILE* fp, void* dst, std::size_t dstSize);


// Read the given number of bytes from a file.
//
// Throws os::Error
void read(std::FILE* fp, void* dst, std::size_t dstSize);


// Read the next line from a file, terminating on either line break
// (\r, \n, or \r\n) or the end of the file. Returns false if the line
// cannot be read because the end of the file is reached.
//
// The function clears the line before performing any action.
//
// Throws os::Error.
bool readLine(std::FILE* fp, std::string& line);


// Write data to a file. Throws os::Error.
void write(std::FILE* fp, const void* data, std::size_t dataSize);
void write(std::FILE* fp, const std::string& str);
void write(std::FILE* fp, const char* str);
void write(std::FILE* fp, char c);


// Remove a regular file. The behavior is platform-specific if
// filePath points to anything other than a regular file.
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


// Synchronize directory with storage device.
//
// The function is an equivalent of syncFile() for directories: it's
// usually called after creating a file to ensure that the new
// directory entry has reached the storage device.
//
// Throws os::Error.
void syncDir(const char* dirPath);


// Run an executable.
//
// If supported by the platform, exePath may be just the name of the
// executable (e.g., to look up in the PATH environment variable).
//
// The function blocks the caller's thread until the executable exits.
// Throws os::Error.
void exec(
    const char* exePath,
    const char* const args[],
    std::size_t numArgs);


}
