
// There's no universal way to find the location of the executable on
// Unix-like systems. Most modern systems provide their own ways for
// that, like procfs on Linux, KERN_PROC_PATHNAME on FreeBSD, etc.
//
// We use argv[0] in the general case. While it's not as robust as a
// platform-specific approach, it's definitely more portable and
// works in all normal invocation scenarios. Let's add
// platform-specific things only when the argv[0] way fails.
//
// On platforms that don't provide a way to query executable path and
// where argv[0] fails at the same time, we can give up and fall back
// to the hardcoded install prefix.

#include "app_dirs.h"
#include "init_app_dirs.h"

#include <string>

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"
#include "dpso_utils/unix/exe_path.h"

#include "app_dirs_unix_cfg.h"
#include "exe_path.h"


static std::string baseDirPath;


namespace ui {


bool initAppDirs()
{
    baseDirPath = getExePath();

    // Skip the executable and the parent dir (bin).
    for (auto i = 0; i < 2; ++i) {
        const auto slashPos = baseDirPath.rfind('/');
        if (slashPos != baseDirPath.npos)
            baseDirPath.resize(slashPos);
    }

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
