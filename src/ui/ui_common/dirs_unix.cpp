
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

#include "dirs.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

#include "dirs_unix_cfg.h"
#include "dpso/error.h"
#include "file_names.h"


static std::string findExeInPath(const char* exeName)
{
    const auto* pathEnv = getenv("PATH");
    if (!pathEnv)
        return {};

    for (const auto* s = pathEnv; *s;) {
        const auto* pathBegin = s;

        while (*s && *s != ':')
            ++s;

        const auto* pathEnd = s;
        if (*s == ':')
            ++s;

        std::string path{pathBegin, pathEnd};
        if (path.empty())
            // Empty path is a legacy feature that indicates the
            // current working directory.
            path += '.';

        path += '/';
        path += exeName;

        if (access(path.c_str(), X_OK) == 0)
            return path;
    }

    return {};
}


static std::string baseDirPath;


int uiInitDirs(const char* argv0)
{
    std::string path;

    if (strchr(argv0, '/'))
        path = argv0;
    else {
        path = findExeInPath(argv0);
        if (path.empty()) {
            dpsoSetError("Can't find \"%s\" in $PATH", argv0);
            return false;
        }
    }

    auto* realPath = realpath(path.c_str(), nullptr);
    if (!realPath) {
        dpsoSetError(
            "realpath(\"%s\") failed: %s",
            path.c_str(), strerror(errno));
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


const char* uiGetDir(UiDir dir)
{
    static std::string result;
    result = baseDirPath + '/';

    switch (dir) {
        case UiDirData:
            result += dataDir;
            break;
        case UiDirDoc:
            result += docDir;
            break;
        case UiDirLocale:
            result += localeDir;
            break;
    }

    return result.c_str();
}
