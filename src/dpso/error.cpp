
#include "error.h"

#include <cstdarg>
#include <string>

#include "str.h"


static std::string lastError;


const char* dpsoGetError(void)
{
    return lastError.c_str();
}


void dpsoSetError(const char* fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    lastError = dpso::str::printf(fmt, args);
    va_end(args);
}
