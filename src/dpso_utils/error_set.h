#pragma once

#include "str.h"


namespace dpso {


// Set an error message to be returned by dpsoGetError().
void setError(
    const char* fmt, std::initializer_list<std::string_view> args);


template<typename... Args>
void setError(const char* fmt, const Args&... args)
{
    setError(fmt, {str::formatArg::get(args)...});
}


}
