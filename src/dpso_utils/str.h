#pragma once

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>


namespace dpso::str {


// is*() functions are replacements for routines from ctype.h. Unlike
// the ctype counterparts, they are locale-independent and prevent you
// from getting undefined behavior by forgetting to add a cast from
// char to unsigned char (the behavior of ctype routines is undefined
// if the passed value is neither equal to EOF nor presentable as
// unsigned char).


bool isBlank(unsigned char c);
bool isSpace(unsigned char c);


int cmpIgnoreCase(std::string_view a, std::string_view b);
bool equalIgnoreCase(std::string_view a, std::string_view b);

bool startsWith(std::string_view s, std::string_view other);
bool endsWith(std::string_view s, std::string_view other);


std::string_view trimLeft(
    std::string_view s, bool (&pred)(unsigned char c));
std::string_view trimRight(
    std::string_view s, bool (&pred)(unsigned char c));
std::string_view trim(
    std::string_view s, bool (&pred)(unsigned char c));


std::string justifyLeft(
    std::string s, std::size_t width, char fill = ' ');
std::string justifyRight(
    std::string s, std::size_t width, char fill = ' ');


// toStr() functions are locale-independent replacements for
// std::to_string(). Since C++26, std::to_string() is the same as
// std::format("{}", v), so these functions will be unnecessary once
// we have C++26.
std::string toStr(char c);
std::string toStr(int v, int base = 10);
std::string toStr(unsigned v, int base = 10);
std::string toStr(long v, int base = 10);
std::string toStr(unsigned long v, int base = 10);
std::string toStr(long long v, int base = 10);
std::string toStr(unsigned long long v, int base = 10);

std::string toStr(float v);
std::string toStr(double v);

std::string toHex(const void* data, std::size_t size);


namespace formatArg {


inline std::string_view get(std::string_view v)
{
    return v;
}


template<typename T>
auto get(const T& v) -> decltype(toStr(v))
{
    return toStr(v);
}


}


// Python-style brace string formatting. Only positional arguments are
// supported.
std::string format(
    std::string_view fmt,
    std::initializer_list<std::string_view> args);

template<typename... Args>
std::string format(std::string_view fmt, const Args&... args)
{
    return format(fmt, {formatArg::get(args)...});
}


}
