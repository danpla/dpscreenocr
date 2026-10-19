#pragma once

#include <cstddef>
#include <cstdint>

#include "dp_intl/error.h"


namespace dp::intl {


/**
 * Input stream.
 *
 * Although the class methods use types such as `size_t` and
 * `int64_t`, the dp_intl library will never read or position the
 * stream beyond the first 100 megabytes. This means that if the
 * underlying API used for a concrete Stream is limited to 32-bit
 * integer types, you don't need to handle edge cases like integer
 * overflow or wrap-around.
 */
class Stream {
public:
    virtual ~Stream() = default;

    /**
     * Read up to `dstSize` bytes from the stream and advance the
     * stream position by the number of bytes read.
     *
     * \returns The number of bytes read, which can be less than
     *     `dstSize` if the end of the stream is reached.
     *
     * \throws dp::intl::Error
     */
    virtual std::size_t readSome(void* dst, std::size_t dstSize) = 0;

    /**
     * Set the stream position for the next read operation.
     *
     * `newPosition` is relative to the beginning of the stream.
     * dp_intl will never call this method with a negative
     * `newPosition`.
     *
     * \throws dp::intl::Error
     */
    virtual void setPosition(std::int64_t newPosition) = 0;
};


}
