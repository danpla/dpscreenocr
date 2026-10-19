#include "str_utils.h"

#include <algorithm>
#include <cstdint>


namespace dp::intl {


unsigned char toUpper(unsigned char c)
{
    if (c < 'a' || c > 'z')
        return c;

    return c - ('a' - 'A');
}


unsigned char toLower(unsigned char c)
{
    if (c < 'A' || c > 'Z')
        return c;

    return c + ('a' - 'A');
}


bool isBlank(unsigned char c)
{
    return c == ' ' || c == '\t';
}


bool isAlpha(unsigned char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}


bool isDigit(unsigned char c)
{
    return c >= '0' && c <= '9';
}


static int cmpIgnoreCase(
    std::string_view a, std::string_view b, std::size_t n)
{
    for (std::size_t i{}; i < n; ++i)
        if (const auto d = toLower(a[i]) - toLower(b[i]); d != 0)
            return d;

    return 0;
}


static int cmpIgnoreCase(std::string_view a, std::string_view b)
{
    const auto d = cmpIgnoreCase(a, b, std::min(a.size(), b.size()));
    if (d != 0)
        return d;

    if (a.size() < b.size())
        return -1;

    if (a.size() > b.size())
        return 1;

    return 0;
}


bool lessIgnoreCase(std::string_view a, std::string_view b)
{
    return cmpIgnoreCase(a, b) < 0;
}


bool equalIgnoreCase(std::string_view a, std::string_view b)
{
    return a.size() == b.size()
        && cmpIgnoreCase(a, b, a.size()) == 0;
}


bool startsWith(std::string_view s, std::string_view other)
{
    return s.size() >= other.size()
        && s.compare(0, other.size(), other) == 0;
}


std::string toHex(const void* data, std::size_t size)
{
    std::string result;
    result.reserve(size * 2);

    const auto* bytes = static_cast<const std::uint8_t*>(data);
    static const auto* hexChars = "0123456789abcdef";

    for (std::size_t i{}; i < size; ++i) {
        const auto b = bytes[i];
        result += hexChars[b >> 4];
        result += hexChars[b & 0b1111];
    }

    return result;
}


}
