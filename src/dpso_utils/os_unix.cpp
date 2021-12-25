
#include "os.h"


const char* const dpsoDirSeparators = "/";


FILE* dpsoFopenUtf8(const char* filePath, const char* mode)
{
    return fopen(filePath, mode);
}
