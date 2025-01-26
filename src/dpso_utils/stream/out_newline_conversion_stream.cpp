#include "stream/out_newline_conversion_stream.h"

#include <algorithm>

#include "os.h"


namespace dpso {


OutNewlineConversionStream::OutNewlineConversionStream(
        Stream& base, const char* newline)
    : base{base}
    , newline{newline ? newline : os::newline}
{
}


std::size_t OutNewlineConversionStream::readSome(
    void* dst, std::size_t dstSize)
{
    return base.readSome(dst, dstSize);
}


static bool isLf(char c)
{
    return c == '\n';
}


void OutNewlineConversionStream::write(
    const void* src, std::size_t srcSize)
{
    if (newline.size() == 1 && isLf(newline[0])) {
        base.write(src, srcSize);
        return;
    }

    const auto* s = static_cast<const char*>(src);
    const auto* sEnd = s + srcSize;

    while (s < sEnd) {
        const auto* lfsBegin = std::find_if(s, sEnd, isLf);
        base.write(s, lfsBegin - s);

        const auto* lfsEnd = std::find_if_not(lfsBegin, sEnd, isLf);

        if (!newline.empty())
            for (auto i = lfsEnd - lfsBegin; i--;)
                base.write(newline.data(), newline.size());

        s = lfsEnd;
    }
}


}
