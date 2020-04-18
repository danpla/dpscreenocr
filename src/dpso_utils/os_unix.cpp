
#include "os.h"


namespace dpso {


const char dirSeparator = '/';


std::FILE* fopenUtf8(const char* fileName, const char* mode)
{
    return std::fopen(fileName, mode);
}


}
