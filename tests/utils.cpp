
#include "utils.h"

#include <cctype>
#include <cstdio>


namespace test {
namespace utils {


std::string boolToStr(bool b)
{
    return b ? "true" : "false";
}


std::string escapeStr(const char* str)
{
    std::string result;

    while (*str) {
        const auto c = *str++;

        switch (c) {
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        case '\\':
            result += "\\\\";
            break;
        default: {
            if (std::isprint(c))
                result += c;
            else {
                char buf[5];
                std::snprintf(buf, sizeof(buf), "\\x%02hhx", c);
                result += buf;
            }
            break;
        }
        }
    }

    return result;
}


}
}
