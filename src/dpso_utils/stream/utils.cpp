#include "stream/utils.h"

#include <cstring>

#include "stream/stream.h"


namespace dpso {


void read(Stream& stream, void* dst, std::size_t dstSize)
{
    if (stream.readSome(dst, dstSize) != dstSize)
        throw StreamError{"Unexpected end of stream"};
}


void write(Stream& stream, const std::string& str)
{
    stream.write(str.data(), str.size());
}


void write(Stream& stream, const char* str)
{
    stream.write(str, std::strlen(str));
}


void write(Stream& stream, char c)
{
    stream.write(&c, 1);
}


}
