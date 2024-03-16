
#include "stream/file_stream.h"

#include <cassert>

#include "os.h"


namespace dpso {


struct FileStream::Impl {
    os::StdFileUPtr fp;
};


static const char* getFopenMode(FileStream::Mode mode)
{
    switch (mode) {
    case FileStream::Mode::read:
        return "rb";
    case FileStream::Mode::write:
        return "wb";
    case FileStream::Mode::append:
        return "ab";
    }

    assert(false);
    return "";
}


FileStream::FileStream(const char* fileName, FileStream::Mode mode)
    : impl{std::make_unique<Impl>()}
{
    impl->fp.reset(os::fopen(fileName, getFopenMode(mode)));
    if (!impl->fp)
        os::throwErrno("fopen()", errno);
}


FileStream::~FileStream() = default;


void FileStream::write(const void* src, std::size_t srcSize)
{
    if (std::fwrite(src, 1, srcSize, impl->fp.get()) != srcSize)
        throw StreamError{"fwrite() failed"};
}


std::size_t FileStream::readSome(void* dst, std::size_t dstSize)
{
    const auto numRead = std::fread(dst, 1, dstSize, impl->fp.get());
    if (std::ferror(impl->fp.get()))
        throw StreamError{"fread() failed"};

    return numRead;
}


void FileStream::sync()
{
    if (std::fflush(impl->fp.get()) == EOF)
        throw os::Error{"fflush() failed"};

    os::syncFile(impl->fp.get());
}


const char* toStr(FileStream::Mode mode)
{
    #define CASE(M) \
        case FileStream::Mode::M: \
            return "FileStream::Mode" #M

    switch (mode) {
        CASE(read);
        CASE(write);
        CASE(append);
    }

    #undef CASE

    assert(false);
    return "";
}


}
