
#include "cfg_path.h"

#include <string>

#include <windows.h>
#include <shlobj.h>

#include "os.h"
#include "windows_utils.h"


const char* dpsoGetCfgPath(const char* appName)
{
    static std::string path;
    // Fall back to the current working directory in case of errors.
    path = ".";

    // We use SHGetFolderPath() for XP support; Vista and later has
    // SHGetKnownFolderPath() for this.
    wchar_t appDataPathUtf16[MAX_PATH];
    if (FAILED(SHGetFolderPathW(
            nullptr, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
            nullptr, SHGFP_TYPE_CURRENT, appDataPathUtf16)))
        return path.c_str();

    std::wstring pathUtf16 = appDataPathUtf16;
    pathUtf16 += L'\\';
    try {
        pathUtf16 += dpso::win::utf8ToUtf16(appName);
    } catch (std::runtime_error&) {
        return path.c_str();
    }

    if (!CreateDirectoryW(pathUtf16.c_str(), nullptr)
            && GetLastError() != ERROR_ALREADY_EXISTS)
        return path.c_str();

    try {
        path = dpso::win::utf16ToUtf8(pathUtf16);
    } catch (std::runtime_error&) {
        return path.c_str();
    }

    return path.c_str();
}
