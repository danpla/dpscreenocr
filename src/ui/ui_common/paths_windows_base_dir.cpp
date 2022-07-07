
#include "paths.h"

#include "dpso/error.h"


static std::string baseDirPath;


int uiInitBaseDirPath(const char* argv0)
{
    (void)argv0;
    #error Implement
    return false;
}


const char* uiGetBaseDirPath(void)
{
    return baseDirPath.c_str();
}
