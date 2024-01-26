
#pragma once

#include <cstddef>
#include <string>


namespace dpso::str {


// is*() functions are replacements for routines from ctype.h. Unlike
// the ctype counterparts, they are locale-independent and prevent you
// from getting undefined behavior by forgetting to add a cast from
// char to unsigned char (the behavior of ctype routines is undefined
// if the passed value is neither equal to EOF nor presentable as
// unsigned char).


bool isBlank(unsigned char c);
bool isSpace(unsigned char c);


enum CmpOption {
    cmpNormal = 0,
    cmpIgnoreCase = 1 << 0,
};


// Compare str with at most subStrLen characters of subStr.
//
// The function allows to compare a string with a part of another
// string without the need to null-terminate the part manually.
//
// It's similar to strncmp(), except that if null is not found in the
// first subStrLen characters of either string, the function
// additionally checks whether str[subStrLen] is null to ensure that
// subStr is not a prefix of str.
//
// For example ("Foo", "FooBar", 3) == 0, but ("FooBar", "Foo", 3)
// > 0.
int cmpSubStr(
    const char* str,
    const char* subStr, std::size_t subStrLen,
    unsigned options = cmpNormal);


inline int cmp(
    const char* a, const char* b, unsigned options = cmpNormal)
{
    return cmpSubStr(a, b, -1, options);
}


// toStr() functions are locale-independent replacements for
// std::to_string(). They all have the same effect as
// fmt::format("{}", v), but are shorter and and don't require
// including <fmt/*> headers.
//
// Since C++26, std::to_string() is the same as std::format("{}", v),
// so these functions will be unnecessary once we have C++26.
std::string toStr(int v);
std::string toStr(unsigned v);
std::string toStr(long v);
std::string toStr(unsigned long v);
std::string toStr(long long v);
std::string toStr(unsigned long long v);


}
