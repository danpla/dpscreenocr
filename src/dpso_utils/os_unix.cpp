
#include "os.h"


const char* const dpsoDirSeparators = "/";


FILE* dpsoFopenUtf8(const char* fileName, const char* mode)
{
    return fopen(fileName, mode);
}
