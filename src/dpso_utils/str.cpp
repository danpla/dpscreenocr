#include "str.h"

#include <charconv>
#include <climits>
#include <cstring>

#include "str_format_core.h"


namespace dpso::str {


bool isBlank(unsigned char c)
{
    return c == '\t' || c == ' ';
}


bool isSpace(unsigned char c)
{
    return c && std::strchr(" \f\n\r\t\v", c);
}


static unsigned char toLower(unsigned char c)
{
    if (c < 'A' || c > 'Z')
        return c;

    return c + ('a' - 'A');
}


int cmpSubStr(
    const char* str,
    const char* subStr, std::size_t subStrLen,
    unsigned options)
{
    for (std::size_t i = 0; i < subStrLen; ++i) {
        unsigned char c1 = str[i];
        unsigned char c2 = subStr[i];

        if (options & cmpIgnoreCase) {
            c1 = toLower(c1);
            c2 = toLower(c2);
        }

        const auto diff = c1 - c2;
        if (diff != 0 || c1 == 0)
            return diff;
    }

    return str[subStrLen] == 0 ? 0 : 1;
}


std::string leftJustify(std::string s, std::size_t width, char fill)
{
    if (s.size() < width)
        s.resize(width, fill);

    return s;
}


std::string rightJustify(std::string s, std::size_t width, char fill)
{
    if (s.size() < width)
        s.insert(0, width - s.size(), fill);

    return s;
}


template<typename T>
static std::string intToStr(T v, int base)
{
    // Reserve space for the base-2 representation.
    char buf[sizeof(T) * CHAR_BIT];
    const auto r = std::to_chars(buf, buf + sizeof(buf), v, base);
    // to_chars() can only fail with std::errc::value_too_large.
    return {buf, r.ptr};
}


std::string toStr(int v, int base)
{
    return intToStr(v, base);
}


std::string toStr(unsigned v, int base)
{
    return intToStr(v, base);
}


std::string toStr(long v, int base)
{
    return intToStr(v, base);
}


std::string toStr(unsigned long v, int base)
{
    return intToStr(v, base);
}


std::string toStr(long long v, int base)
{
    return intToStr(v, base);
}


std::string toStr(unsigned long long v, int base)
{
    return intToStr(v, base);
}


// At the time of writing, not all compilers have support for the
// floating-point std::to_chars overloads, so we use std::to_string
// and manually replace a decimal comma (if emitted by the current
// locale) with a period.
static std::string doubleToStr(double v)
{
    auto result = std::to_string(v);

    const auto lastNonzero = result.find_last_not_of('0');
    const auto sep = result.find_last_of(".,", lastNonzero);

    if (sep == result.npos)
        return result;

    if (sep == lastNonzero) {
        // Only zeros after the decimal separator.
        result.resize(sep);
        return result;
    }

    result[sep] = '.';

    if (lastNonzero != result.npos)
        result.resize(lastNonzero + 1);

    return result;
}


std::string toStr(float v)
{
    return doubleToStr(v);
}


std::string toStr(double v)
{
    return doubleToStr(v);
}


namespace formatArg {


const char* get(char* v)
{
    return v;
}


const char* get(const char* v)
{
    return v;
}


const char* get(const std::string& v)
{
    return v.c_str();
}


ConvertedStr get(char c)
{
    return {std::string(1, c)};
}


}


std::string format(
    const char* fmt, std::initializer_list<const char*> args)
{
    std::size_t nextIdx{};

    return format(
        fmt,
        [&](const char* /*name*/, std::size_t nameLen) -> const char*
        {
            if (nameLen != 0)
                return {};

            const auto idx = nextIdx++;
            return idx < args.size() ? args.begin()[idx] : nullptr;
        });
}


}
