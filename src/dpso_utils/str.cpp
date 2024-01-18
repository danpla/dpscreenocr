
#include "str.h"

#include <cstring>


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


}
