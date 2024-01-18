
#pragma once

#include <fmt/core.h>


namespace dpso {


// Set an error message to be returned by dpsoGetError().
void vSetError(fmt::string_view format, fmt::format_args args);


template<typename... T>
void setError(fmt::format_string<T...> format, T&&... args)
{
    vSetError(format, fmt::make_format_args(args...));
}


}
