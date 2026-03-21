#pragma once

#include "str.h"


namespace dpso {


// Set an error message to be returned by dpsoGetError().
void setError(
    std::string_view fmt,
    std::initializer_list<std::string_view> args);


template<typename... Args>
void setError(std::string_view fmt, const Args&... args)
{
    setError(fmt, {str::formatArg::get(args)...});
}


}
