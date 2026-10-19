#pragma once

#include "dp_intl/error.h"
#include "dp_intl/stream.h"


namespace dp::intl {


inline void read(Stream& stream, void* dst, std::size_t dstSize)
{
    if (stream.readSome(dst, dstSize) != dstSize)
        throw Error{"Unexpected end of the stream"};
}


}
