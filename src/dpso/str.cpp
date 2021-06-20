
#include "str.h"

#include <cctype>


namespace dpso {
namespace str {


int cmpSubStr(
    const char* str,
    const char* subStr, std::size_t subStrLen,
    unsigned options)
{
    for (std::size_t i = 0; i < subStrLen; ++i) {
        unsigned char c1 = str[i];
        unsigned char c2 = subStr[i];

        if (options & cmpIgnoreCase) {
            c1 = std::tolower(c1);
            c2 = std::tolower(c2);
        }

        const auto diff = c1 - c2;
        if (diff != 0 || c1 == 0)
            return diff;
    }

    return str[subStrLen] == 0 ? 0 : 1;
}


}
}
