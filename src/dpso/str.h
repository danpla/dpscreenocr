
#pragma once

#include <cstdarg>
#include <cstddef>
#include <string>

#include "printf_fn.h"


namespace dpso {
namespace str {


enum CmpOption {
    cmpNormal = 0,
    cmpIgnoreCase = 1 << 0,
};


/**
 * Compare str with at most subStrLen characters of subStr.
 *
 * The function allows to compare a string with a part of another
 * string without the need to null-terminate the part manually.
 *
 * It's similar to strncmp(), except that if null is not found in the
 * first subStrLen characters of either string, the function
 * additionally checks whether str[subStrLen] is null to ensure that
 * subStr is not a prefix of str.
 *
 * For example ("Foo", "FooBar", 3) == 0, but ("FooBar", "Foo", 3)
 * > 0.
 */
int cmpSubStr(
    const char* str,
    const char* subStr, std::size_t subStrLen,
    unsigned options = cmpNormal);


inline int cmp(
    const char* a, const char* b, unsigned options = cmpNormal)
{
    return cmpSubStr(a, b, -1, options);
}


std::string vprintf(const char* fmt, std::va_list vlist);
std::string printf(const char* fmt, ...) DPSO_PRINTF_FN(1);


}
}
