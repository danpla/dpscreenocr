
#pragma once

#include <cstddef>
#include <string>

#include "dpso_utils/stream/stream.h"


namespace dpso {


class StringStream : public Stream {
public:
    StringStream() = default;

    // Construct the stream with initial data. The stream position
    // will be 0.
    explicit StringStream(const char* str);

    const std::string& getStr() const;
    std::string takeStr();

    std::size_t readSome(void* dst, std::size_t dstSize) override;
    void write(const void* src, std::size_t srcSize) override;
private:
    std::string str;
    std::size_t pos{};
};


}
