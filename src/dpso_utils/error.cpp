
#include "error_get.h"
#include "error_set.h"

#include <cstdarg>
#include <string>

#include "str.h"


static thread_local std::string error;


const char* dpsoGetError(void)
{
    return error.c_str();
}


namespace dpso {


void setError(
    const char* fmt, std::initializer_list<const char*> args)
{
    error = str::format(fmt, args);
}


}
