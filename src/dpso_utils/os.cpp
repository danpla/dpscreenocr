
#include "os.h"


namespace dpso {


std::FILE* fopenUtf8(const char* fileName, const char* mode)
{
    #ifdef __unix__

    return std::fopen(fileName, mode);

    #else

    #error "Not implemented"

    #endif
}


}
