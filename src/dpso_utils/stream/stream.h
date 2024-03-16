
#pragma once

#include <cstddef>
#include <stdexcept>


namespace dpso {


class StreamError : public std::runtime_error {
    using runtime_error::runtime_error;
};


class Stream {
public:
    virtual ~Stream() = default;

    // Read up to dstSize bytes from the stream. Returns the number of
    // bytes read, which may be less than dstSize if the end of the
    // stream is reached.
    //
    // Throws StreamError.
    virtual std::size_t readSome(void* dst, std::size_t dstSize) = 0;

    // Throws StreamError.
    virtual void write(const void* src, std::size_t srcSize) = 0;
};


}
