
#include "os.h"


namespace dpso {


std::FILE* fopenUtf8(const char* fileName, const char* mode)
{
    return std::fopen(fileName, mode);
}


}
