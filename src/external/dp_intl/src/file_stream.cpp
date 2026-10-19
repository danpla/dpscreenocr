#include "dp_intl/file_stream.h"

#include <cstdio>
#include <cerrno>
#include <cstring>


namespace dp::intl {


static std::FILE* toFp(void* impl)
{
    return static_cast<std::FILE*>(impl);
}


FileStream::FileStream(const std::filesystem::path& path)
    : impl{
        #ifdef _WIN32
        _wfopen(path.c_str(), L"rb")
        #else
        std::fopen(path.c_str(), "rb")
        #endif
    }
{
    if (impl)
        return;

    const auto* errorStr = std::strerror(errno);
    if (errno == ENOENT)
        throw FileNotFoundError{errorStr};

    throw Error{errorStr};
}


FileStream::~FileStream()
{
    std::fclose(toFp(impl));
}


std::size_t FileStream::readSome(void* dst, std::size_t dstSize)
{
    const auto numRead = std::fread(dst, 1, dstSize, toFp(impl));
    if (std::ferror(toFp(impl)))
        throw Error{"fread() failed"};

    return numRead;
}


void FileStream::setPosition(std::int64_t newPosition)
{
    if (std::fseek(toFp(impl), newPosition, SEEK_SET) != 0)
        throw Error{"fseek(..., SEEK_SET) failed"};
}


}
