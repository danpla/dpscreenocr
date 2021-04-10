
#include "utils.h"

#include <cctype>


namespace test {
namespace utils {


std::string escapeStr(const char* str)
{
    std::string result;

    while (*str) {
        const auto c = *str;
        ++str;

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
                if (std::isprint(c)) {
                    static char buf[5];
                    std::snprintf(
                        buf, sizeof(buf),
                        "\\x%.2x", static_cast<unsigned char>(c));
                    result += buf;
                } else
                    result += c;
                break;
            }
        }
    }

    return result;
}


}
}
