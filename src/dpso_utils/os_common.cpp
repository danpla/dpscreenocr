
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


std::size_t readSome(std::FILE* fp, void* dst, std::size_t dstSize)
{
    const auto numRead = std::fread(dst, 1, dstSize, fp);
    if (std::ferror(fp))
        throw Error("fread() failed");

    return numRead;
}


void read(std::FILE* fp, void* dst, std::size_t dstSize)
{
    if (readSome(fp, dst, dstSize) != dstSize)
        throw Error("Unexpected end of file");
}


static int checkedFgetc(std::FILE* fp)
{
    const auto c = std::fgetc(fp);
    if (c == EOF && std::ferror(fp))
        throw Error{"fgetc() failed"};

    return c;
}


bool readLine(std::FILE* fp, std::string& line)
{
    line.clear();

    while (true) {
        const auto c = checkedFgetc(fp);
        if (c == EOF)
            return !line.empty();

        if (c == '\r' || c == '\n') {
            if (c == '\r')
                if (const auto c2 = checkedFgetc(fp); c2 != '\n')
                    std::ungetc(c2, fp);

            break;
        }

        line += c;
    }

    return true;
}


void write(std::FILE* fp, const void* data, std::size_t dataSize)
{
    if (std::fwrite(data, 1, dataSize, fp) != dataSize)
        throw Error{"fwrite() failed"};
}


void write(std::FILE* fp, const std::string& str)
{
    write(fp, str.data(), str.size());
}


void write(std::FILE* fp, const char* str)
{
    write(fp, str, std::strlen(str));
}


void write(std::FILE* fp, char c)
{
    write(fp, &c, 1);
}


}
