#include "str_utils.h"

#include <algorithm>


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


const std::string_view blanks{" \t"};


bool isBlank(unsigned char c)
{
    return blanks.find(c) != blanks.npos;
}


static std::string_view trimLeftBlanks(std::string_view s)
{
    if (const auto p = s.find_first_not_of(blanks); p != s.npos)
        return s.substr(p);

    return {};
}


static std::string_view trimRightBlanks(std::string_view s)
{
    if (const auto p = s.find_last_not_of(blanks); p != s.npos)
        return s.substr(0, p + 1);

    return {};
}


std::string_view trimBlanks(std::string_view s)
{
    return trimLeftBlanks(trimRightBlanks(s));
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
    const auto r = cmpIgnoreCase(a, b, std::min(a.size(), b.size()));
    if (r != 0)
        return r;

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


}
