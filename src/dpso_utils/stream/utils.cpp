#include "stream/utils.h"

#include "stream/stream.h"


namespace dpso {


void read(Stream& stream, void* dst, std::size_t dstSize)
{
    if (stream.readSome(dst, dstSize) != dstSize)
        throw StreamError{"Unexpected end of stream"};
}


void write(Stream& stream, std::string_view str)
{
    stream.write(str.data(), str.size());
}


void write(Stream& stream, char c)
{
    stream.write(&c, 1);
}


}
