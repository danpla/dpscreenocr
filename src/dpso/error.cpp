
#include "error.h"

#include <cstdarg>
#include <cstdio>
#include <string>


static std::string lastError;


const char* dpsoGetError(void)
{
    return lastError.c_str();
}


void dpsoSetError(const char* fmt, ...)
{
    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);

    const auto size = std::vsnprintf(nullptr, 0, fmt, args1);
    va_end(args1);
    if (size < 0) {
        va_end(args2);

        lastError = (
            std::string{"dpsoSetError(\""}
            + fmt + "\", ...): vsnprintf() error");

        return;
    }

    // Don't write to lastError directly since it can currently be in
    // args, e.g. dpsoSetError("%s", dpsoGetError()).
    std::string error;
    error.resize(size);
    // C++ standard allows to overwrite string[size()] with CharT().
    std::vsnprintf(&error[0], size, fmt, args2);
    va_end(args2);

    lastError.swap(error);
}
