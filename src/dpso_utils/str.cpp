
#include "str.h"

#include <charconv>
#include <cstring>
#include <limits>

#include "os.h"


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


std::string lfToNativeNewline(const char* str)
{
    std::string result;

    for (const auto* s = str; *s; ++s)
        if (*s == '\n')
            result += DPSO_OS_NEWLINE;
        else
            result += *s;

    return result;
}


template<typename T>
static std::string intToStr(T v)
{
    // Add extra 2 for:
    // * +1 for numeric_limits::digits10. For example, digits10 is 2
    //   for a 8-bit unsigned, because it can represent any two-digit
    //   number, but not 256-999.
    // * +1 for a possible minus sign.
    char buf[std::numeric_limits<T>::digits10 + 2];
    const auto r = std::to_chars(buf, buf + sizeof(buf), v, 10);
    // to_chars() can only fail with std::errc::value_too_large.
    return {buf, r.ptr};
}


std::string toStr(int v)
{
    return intToStr(v);
}


std::string toStr(unsigned v)
{
    return intToStr(v);
}


std::string toStr(long v)
{
    return intToStr(v);
}


std::string toStr(unsigned long v)
{
    return intToStr(v);
}


std::string toStr(long long v)
{
    return intToStr(v);
}


std::string toStr(unsigned long long v)
{
    return intToStr(v);
}


}
