#include "error_get.h"
#include "error_set.h"


static thread_local std::string error;


const char* dpsoGetError(void)
{
    return error.c_str();
}


namespace dpso {


void setError(
    std::string_view fmt,
    std::initializer_list<std::string_view> args)
{
    error = str::format(fmt, args);
}


}
