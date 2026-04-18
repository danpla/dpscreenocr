#include "app_dirs.h"

#include "exe_path.h"


static std::string calcBaseDirPath()
{
    auto result = ui::getExePath();

    const auto slashPos = result.rfind('\\');
    if (slashPos != result.npos)
        result.resize(slashPos);

    return result;
}


const char* uiGetAppDir(UiAppDir dir)
{
    static const auto baseDirPath = calcBaseDirPath();

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
