#pragma once

#include <memory>

#include "dpso_utils/stream/stream.h"


namespace dpso {


class FileStream : public Stream {
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
    FileStream(const char* fileName, Mode mode);
    ~FileStream();

    FileStream(const FileStream&) = delete;
    FileStream& operator=(const FileStream&) = delete;

    FileStream(FileStream&&) = delete;
    FileStream& operator=(FileStream&&) = delete;

    std::size_t readSome(void* dst, std::size_t dstSize) override;

    void write(const void* src, std::size_t srcSize) override;

    // Synchronize the file state with the storage device.
    //
    // Throws os::Error.
    void sync();
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


const char* toStr(FileStream::Mode mode);


}
