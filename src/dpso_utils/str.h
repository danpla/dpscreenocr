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


std::string leftJustify(
    std::string s, std::size_t width, char fill = ' ');


std::string rightJustify(
    std::string s, std::size_t width, char fill = ' ');


// toStr() functions are locale-independent replacements for
// std::to_string(). Since C++26, std::to_string() is the same as
// std::format("{}", v), so these functions will be unnecessary once
// we have C++26.
std::string toStr(int v, int base = 10);
std::string toStr(unsigned v, int base = 10);
std::string toStr(long v, int base = 10);
std::string toStr(unsigned long v, int base = 10);
std::string toStr(long long v, int base = 10);
std::string toStr(unsigned long long v, int base = 10);

std::string toStr(float v);
std::string toStr(double v);


namespace formatArg {


// The "char*" overload is necessary because the template version of
// get() has a higher priority than the "const char*" overload.
const char* get(char* v);
const char* get(const char* v);
const char* get(const std::string& v);


struct ConvertedStr {
    std::string s;

    operator const char*() const {
        return s.c_str();
    }
};


ConvertedStr get(char c);


template<typename T>
ConvertedStr get(const T& v)
{
    return {toStr(v)};
}


}


// Python-style brace string formatting. Only positional arguments are
// supported.
std::string format(
    const char* fmt, std::initializer_list<const char*> args);

template<typename... Args>
std::string format(const char* fmt, const Args&... args)
{
    return format(fmt, {formatArg::get(args)...});
}


}
