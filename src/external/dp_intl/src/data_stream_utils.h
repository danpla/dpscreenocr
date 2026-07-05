#pragma once

#include "dp_intl/data_stream.h"
#include "dp_intl/error.h"


namespace dp::intl {


inline void read(DataStream& stream, void* dst, std::size_t dstSize)
{
    if (stream.readSome(dst, dstSize) != dstSize)
        throw Error{"Unexpected end of data stream"};
}


}
