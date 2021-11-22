
#include "error.h"

#include <string>


static std::string lastError;


const char* dpsoGetError(void)
{
    return lastError.c_str();
}


void dpsoSetError(const char* error)
{
    lastError = error;
}
