#pragma once

#include <cstdio>

#include "str.h"


namespace dpso::str {


void print(
    std::FILE* fp,
    std::string_view fmt,
    std::initializer_list<std::string_view> args);


template<typename... Args>
void print(std::FILE* fp, std::string_view fmt, const Args&... args)
{
    print(fp, fmt, {str::formatArg::get(args)...});
}


inline void print(
    std::string_view fmt,
    std::initializer_list<std::string_view> args)
{
    print(stdout, fmt, args);
}


template<typename... Args>
void print(std::string_view fmt, const Args&... args)
{
    print(fmt, {str::formatArg::get(args)...});
}


}
