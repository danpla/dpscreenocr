#include "app_dirs.h"
#include "init_app_dirs.h"

#include <string>

#include "app_dirs_unix_cfg.h"
#include "exe_path.h"
#include "toplevel_argv0.h"


static std::string baseDirPath;


namespace ui {


bool initAppDirs()
{
    baseDirPath = getExePath();

    // Drop the executable name.
    if (const auto p = baseDirPath.rfind('/'); p != baseDirPath.npos)
        baseDirPath.resize(p);

    if (!getToplevelArgv0({}).empty())
        // The app was run by the launcher that is already located in
        // the base directory.
        return true;

    // Drop the parent dir (bin).
    if (const auto p = baseDirPath.rfind('/'); p != baseDirPath.npos)
        baseDirPath.resize(p);

    return true;
}


}


const char* uiGetAppDir(UiAppDir dir)
{
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
