
#include "cfg_path.h"

#include <string>

#include "os.h"


FILE* dpsoCfgPathFopen(
    const char* appName, const char* fileName, const char* mode)
{
    std::string path = dpsoGetCfgPath(appName);
    path += dpso::dirSeparator;
    path += fileName;
    return dpso::fopenUtf8(path.c_str(), mode);
}
