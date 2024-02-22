
#pragma once

#include <cstddef>
#include <memory>
#include <string>


namespace dpso {


class File {
public:
    enum class Mode {
        // Open an existing file for reading.
        read,

        // Open a file for writing. If the file already exists, it
        // will be truncated.
        write,

        // Open a file for appending: all write operations will always
        // occur at the end of the file. If the file already exists,
        // its contents are preserved.
        append
    };

    // Throws os::Error.
    File(const char* fileName, Mode mode);
    ~File();

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    File(File&&) = delete;
    File& operator=(File&&) = delete;

    // Read up to dstSize bytes from a file. Returns the number of
    // bytes read, which may be less than dstSize if the end of the
    // file is reached.
    //
    // Throws os::Error.
    std::size_t readSome(void* dst, std::size_t dstSize);

    // Throws os::Error.
    void write(const void* src, std::size_t srcSize);

    // Synchronize the file state with the storage device.
    //
    // Throws os::Error.
    void sync();
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


const char* toStr(File::Mode mode);


// Read the given number of bytes from a file. Throws os::Error.
void read(File& file, void* dst, std::size_t dstSize);


// Write data to a file. Throws os::Error.
void write(File& file, const std::string& str);
void write(File& file, const char* str);
void write(File& file, char c);


}
