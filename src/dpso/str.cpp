
#include "str.h"

#include <cctype>


namespace dpso {
namespace str {


int cmpSubStr(
    const char* str,
    const char* subStr, std::size_t subStrLen,
    bool ignoreCase)
{
    for (std::size_t i = 0; i < subStrLen; ++i) {
        auto c1 = str[i];
        auto c2 = subStr[i];

        if (ignoreCase) {
            c1 = std::tolower(c1);
            c2 = std::tolower(c2);
        }

        const int diff = (
            static_cast<unsigned char>(c1)
            - static_cast<unsigned char>(c2));
        if (diff != 0 || c1 == 0)
            return diff;
    }

    return str[subStrLen] == 0 ? 0 : 1;
}


}
}
