
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"

#include "app_dirs_unix_cfg.h"


static std::string findExeInPath(const char* exeName)
{
    const auto* p = getenv("PATH");
    if (!p || !*p)
        return {};

    while (true) {
        const auto* pathBegin = p;
        while (*p && *p != ':')
            ++p;
        const auto* pathEnd = p;

        std::string path{pathBegin, pathEnd};

        // An empty path is a legacy feature indicating the current
        // working directory. It can appear as two colons in the
        // middle of the list, as well a colon at the beginning or end
        // of the list.
        if (!path.empty() && path.back() != '/')
            path += '/';

        path += exeName;

        if (access(path.c_str(), X_OK) == 0)
            return path;

        if (!*p)
            break;

        ++p;
    }

    return {};
}


static std::string baseDirPath;


bool uiInitAppDirs(const char* argv0)
{
    std::string path;

    if (strchr(argv0, '/'))
        path = argv0;
    else {
        path = findExeInPath(argv0);
        if (path.empty()) {
            dpso::setError("Can't find \"{}\" in $PATH", argv0);
            return false;
        }
    }

    auto* realPath = realpath(path.c_str(), nullptr);
    if (!realPath) {
        dpso::setError(
            "realpath(\"{}\"): {}",
            path, dpso::os::getErrnoMsg(errno));
        return false;
    }

    baseDirPath = realPath;
    free(realPath);

    // Skip the executable and the parent dir (bin).
    for (auto i = 0; i < 2; ++i) {
        const auto slashPos = baseDirPath.rfind('/');
        if (slashPos != baseDirPath.npos)
            baseDirPath.resize(slashPos);
    }

    return true;
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
