
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


void vSetError(fmt::string_view format, fmt::format_args args)
{
    error = fmt::vformat(format, args);
}


}
