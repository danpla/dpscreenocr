#include "utf.h"

#include "dp_intl/error.h"

#include "str_utils.h"


namespace dp::intl {
namespace {


std::string formatCp(char32_t cp)
{
    auto s = toHex(cp);

    for (auto& c : s)
        c = toUpper(c);

    if (s.size() < 4)
        s.insert(0, 4 - s.size(), '0');

    return "U+" + s;
}


std::string formatBytes(const void* data, std::size_t size)
{
    return "0x" + toHex(data, size);
}


std::string formatByte(unsigned char b)
{
    return formatBytes(&b, 1);
}


const char32_t maxCp{0x10ffff};


bool isSurrogate(char32_t cp)
{
    return cp >= 0xd800 && cp <= 0xdfff;
}


// All values are from The Unicode Standard, Chapter 3 Conformance


bool isTrail(unsigned char c)
{
    return (c & 0b11000000) == 0b10000000;
}


struct SequenceInfo {
    unsigned char prefixMask;
    int numTrails;
    char32_t minCp;

    static const SequenceInfo& get(unsigned char lead)
    {
        static const SequenceInfo infos[]{
            {0b10000000, 0, 0},
            {0b11100000, 1, 0x80},
            {0b11110000, 2, 0x800},
            {0b11111000, 3, 0x10000}
        };

        for (const auto& info : infos)
            if ((lead & info.prefixMask)
                    == ((info.prefixMask << 1) & 0xff))
                return info;

        throw Error{"Invalid leading byte " + formatByte(lead)};
    }
};


char32_t decodeUtf8(const char*& seq, const char* end)
{
    if (seq == end)
        throw Error{"Empty string"};

    const unsigned char lead = *seq++;

    const auto& seqInfo = SequenceInfo::get(lead);

    if (end - seq < seqInfo.numTrails)
        throw Error{
            "Truncated " + toStr(1 + seqInfo.numTrails)
            + "-byte sequence"};

    char32_t cp = lead & ~seqInfo.prefixMask;
    for (int i{}; i < seqInfo.numTrails; ++i) {
        const unsigned char trail = *seq++;
        if (!isTrail(trail))
            throw Error{
                "Invalid trailing byte " + formatByte(trail) + " at "
                "index " + toStr(i + 1)};

        cp = (cp << 6) | (trail & 0b111111);
    }

    if (cp < seqInfo.minCp)
        throw Error{"Overlong encoding for " + formatCp(cp)};

    if (isSurrogate(cp))
        throw Error{"Surrogate " + formatCp(cp)};

    if (cp > maxCp)
        throw Error{formatCp(cp) + " exceeds " + formatCp(maxCp)};

    return cp;
}


}


void validateUtf8(std::string_view s)
{
    const auto* begin = s.data();
    const auto* end = begin + s.size();

    for (const auto* seq = begin; seq < end;) {
        const auto* seqStart = seq;

        try {
            decodeUtf8(seq, end);
        } catch (const Error& e) {
            throw Error{
                "Invalid sequence "
                + formatBytes(seqStart, seq - seqStart) + " at index "
                + toStr(seqStart - begin) + ": " + e.what()};
        }
    }
}


}
