
#pragma once

#include <cstddef>
#include <string>


namespace dpso::str {


// This is the function that implements a Python-style brace string
// formatting. The argument lookup is delegated to FindArg, which has
// the following signature:
//
//   const char* findArg(const char* name, std::size_t nameLen)
//
// FindArg can return null if an argument with the given name is not
// found.
template<typename FindArg>
std::string format(const char* fmt, FindArg findArg)
{
    std::string result;

    for (const auto* s = fmt; *s;) {
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

                const auto* arg = findArg(
                    nameBegin, nameEnd - nameBegin);
                if (arg)
                    result += arg;
                else {
                    result += '{';
                    result.append(nameBegin, nameEnd);
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

    return result;
}


}
