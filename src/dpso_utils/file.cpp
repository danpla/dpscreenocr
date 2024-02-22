
#include "file.h"

#include <cassert>
#include <cstring>

#include "os.h"


namespace dpso {


struct File::Impl {
    os::StdFileUPtr fp;
};


static const char* getFopenMode(File::Mode mode)
{
    switch (mode) {
    case File::Mode::read:
        return "rb";
    case File::Mode::write:
        return "wb";
    case File::Mode::append:
        return "ab";
    }

    assert(false);
    return "";
}


File::File(const char* fileName, File::Mode mode)
    : impl{std::make_unique<Impl>()}
{
    impl->fp.reset(os::fopen(fileName, getFopenMode(mode)));
    if (!impl->fp)
        os::throwErrno("fopen()", errno);
}


File::~File() = default;


void File::write(const void* src, std::size_t srcSize)
{
    if (std::fwrite(src, 1, srcSize, impl->fp.get()) != srcSize)
        throw os::Error{"fwrite() failed"};
}


std::size_t File::readSome(void* dst, std::size_t dstSize)
{
    const auto numRead = std::fread(dst, 1, dstSize, impl->fp.get());
    if (std::ferror(impl->fp.get()))
        throw os::Error{"fread() failed"};

    return numRead;
}


void File::sync()
{
    if (std::fflush(impl->fp.get()) == EOF)
        throw os::Error{"fflush() failed"};

    os::syncFile(impl->fp.get());
}


const char* toStr(File::Mode mode)
{
    #define CASE(M) case File::Mode::M: return "File::Mode" #M

    switch (mode) {
        CASE(read);
        CASE(write);
        CASE(append);
    }

    #undef CASE

    assert(false);
    return "";
}


void read(File& file, void* dst, std::size_t dstSize)
{
    if (file.readSome(dst, dstSize) != dstSize)
        throw os::Error("Unexpected end of file");
}


void write(File& file, const std::string& str)
{
    file.write(str.data(), str.size());
}


void write(File& file, const char* str)
{
    file.write(str, std::strlen(str));
}


void write(File& file, char c)
{
    file.write(&c, 1);
}


}
