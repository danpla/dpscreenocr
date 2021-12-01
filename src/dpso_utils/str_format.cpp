
#include "str_format.h"

#include <cstddef>
#include <string>

#include "dpso/str.h"


static const char* findArg(
    const char* name, std::size_t nameLen,
    const struct DpsoFormatArg* args, std::size_t numArgs)
{
    for (std::size_t i = 0; i < numArgs; ++i)
        if (dpso::str::cmpSubStr(args[i].name, name, nameLen) == 0)
            return args[i].str;

    return nullptr;
}


const char* dpsoStrNamedFormat(
    const char* str,
    const struct DpsoFormatArg* args, size_t numArgs)
{
    static std::string result;
    result.clear();

    const auto* s = str;
    while (*s) {
        if (*s == '{') {
            ++s;

            if (*s == '{') {
                result += *s++;
                continue;
            }

            const auto* nameBegin = s;
            while (*s && *s != '{' && *s != '}')
                ++s;
            const auto* nameEnd = s;

            if (*nameEnd == '}') {
                ++s;

                const auto* argStr = findArg(
                    nameBegin, nameEnd - nameBegin,
                    args, numArgs);
                if (argStr)
                    result += argStr;
                else {
                    // If no arg with such name found, leave it as is.
                    result += '{';
                    result.append(nameBegin, nameEnd - nameBegin);
                    result += '}';
                }
            } else {
                // No closing } or unexpected {
                result += '{';
                result += nameBegin;
                break;
            }
        } else if (*s == '}') {
            if (s[1] == '}') {
                result += *s;
                s += 2;
                continue;
            } else {
                // Stray }
                result += s;
                break;
            }
        } else
            result += *s++;
    }

    return result.c_str();
}
