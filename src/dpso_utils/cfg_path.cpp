
#include "cfg_path.h"

#include <string>

#include "os.h"


FILE* dpsoCfgPathFopen(
    const char* appName, const char* baseName, const char* mode)
{
    std::string path = dpsoGetCfgPath(appName);
    path += *dpsoDirSeparators;
    path += baseName;
    return dpsoFopenUtf8(path.c_str(), mode);
}
