
#pragma once

#include <string>

#include "dpso_utils/stream/stream.h"


namespace dpso {


// A stream that replaces all line feeds (\n) with the given newline
// string when writing. Does no conversion when reading.
class OutNewlineConversionStream : public Stream {
public:
    // The newline can be any string. If null, the native newline for
    // the current platform is used.
    explicit OutNewlineConversionStream(
        Stream& base, const char* newline = nullptr);

    std::size_t readSome(void* dst, std::size_t dstSize) override;
    void write(const void* src, std::size_t srcSize) override;
private:
    Stream& base;
    std::string newline;
};


}
