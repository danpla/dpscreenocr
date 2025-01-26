#include "stream/string_stream.h"

#include <algorithm>
#include <cassert>
#include <utility>


namespace dpso {


StringStream::StringStream(const char* str)
    : str{str}
{
}


StringStream::StringStream(StringStream&& other) noexcept
{
    *this = std::move(other);
}


StringStream& StringStream::operator=(StringStream&& other) noexcept
{
    if (this != &other) {
        str = std::exchange(other.str, {});
        pos = std::exchange(other.pos, {});
    }

    return *this;
}


const std::string& StringStream::getStr() const
{
    return str;
}


std::string StringStream::takeStr()
{
    pos = 0;
    return std::exchange(str, {});
}


std::size_t StringStream::readSome(void* dst, std::size_t dstSize)
{
    assert(pos <= str.size());
    const auto numRead = std::min(dstSize, str.size() - pos);

    std::copy_n(str.data() + pos, numRead, static_cast<char*>(dst));
    pos += numRead;

    return numRead;
}


void StringStream::write(const void* src, std::size_t srcSize)
{
    str.resize(std::max(pos + srcSize, str.size()));
    std::copy_n(
        static_cast<const char*>(src), srcSize, str.data() + pos);
    pos += srcSize;
}


}
