
#include "str.h"

#include <cctype>
#include <cstdio>


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


std::string vprintf(const char* fmt, std::va_list args)
{
    std::va_list args2;
    va_copy(args2, args);
    const auto size = std::vsnprintf(nullptr, 0, fmt, args2);
    va_end(args2);

    if (size < 0)
        return {};

    std::string result(size, 0);
    // C++ standard allows to overwrite string[size()] with 0.
    std::vsnprintf(&result[0], size + 1, fmt, args);

    return result;
}


std::string printf(const char* fmt, ...)
{
    std::va_list args;

    va_start(args, fmt);
    auto result = str::vprintf(fmt, args);
    va_end(args);

    return result;
}


}
}
