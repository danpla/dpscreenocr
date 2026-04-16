#include "app_dirs.h"

#include "app_dirs_unix_cfg.h"
#include "exe_path.h"
#include "toplevel_argv0.h"


static std::string calcBaseDirPath()
{
    auto result = ui::getExePath();

    // Drop the executable name.
    if (const auto p = result.rfind('/'); p != result.npos)
        result.resize(p);

    if (!ui::getToplevelArgv0({}).empty())
        // The app was run by the launcher that is already located in
        // the base directory.
        return result;

    // Drop the parent dir (bin).
    if (const auto p = result.rfind('/'); p != result.npos)
        result.resize(p);

    return result;
}


const char* uiGetAppDir(UiAppDir dir)
{
    static const auto baseDirPath = calcBaseDirPath();

    static std::string result;
    result = baseDirPath + '/';

    switch (dir) {
    case UiAppDirData:
        result += unixAppDirData;
        break;
    case UiAppDirDoc:
        result += unixAppDirDoc;
        break;
    case UiAppDirLocale:
        result += unixAppDirLocale;
        break;
    }

    return result.c_str();
}
