
#include "error.h"

#include <cstdarg>
#include <string>

#include "str.h"


static thread_local std::string error;


const char* dpsoGetError(void)
{
    return error.c_str();
}


void dpsoSetError(const char* fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    error = dpso::str::vprintf(fmt, args);
    va_end(args);
}
