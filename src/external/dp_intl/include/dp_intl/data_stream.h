#pragma once

#include <cstddef>
#include <cstdint>

#include "dp_intl/error.h"


namespace dp::intl {


/**
 * Input data stream.
 *
 * This class is an adapter for a file-like data source.
 *
 * Although the class methods use types such as `size_t` and
 * `int64_t`, the dp_intl library will never read or position a file
 * beyond the first 100 megabytes. This means that if the underlying
 * API used for a concrete DataStream is limited to smaller integer
 * types, you don't need to handle the edge cases, such as integer
 * overflow or wrap-around.
 */
class DataStream {
public:
    virtual ~DataStream() = default;

    /**
     * Read up to the given number of bytes from the stream.
     *
     * \returns The number of bytes read, which can be less than
     *     `dstSize` if the end of the stream is reached.
     *
     * \throws Error
     */
    virtual std::size_t readSome(void* dst, std::size_t dstSize) = 0;

    /**
     * Set the stream position.
     *
     * \throws Error
     */
    virtual void setPosition(std::int64_t newPosition) = 0;
};


}
