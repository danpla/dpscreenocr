
#include "app_dirs.h"
#include "init_app_dirs.h"

#include "exe_path.h"


static std::string baseDirPath;


namespace ui {


bool initAppDirs()
{
    baseDirPath = getExePath();

    const auto slashPos = baseDirPath.rfind('\\');
    if (slashPos != baseDirPath.npos)
        baseDirPath.resize(slashPos);

    return true;
}


}


const char* uiGetAppDir(UiAppDir dir)
{
    static std::string result;
    result = baseDirPath;

    switch (dir) {
    case UiAppDirData:
        break;
    case UiAppDirDoc:
        result += "\\doc";
        break;
    case UiAppDirLocale:
        result += "\\locale";
        break;
    }

    return result.c_str();
}
