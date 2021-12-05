
#include "cfg_path.h"

#include <string>

#include <windows.h>
#include <shlobj.h>

#include "dpso/backend/windows/utils.h"
#include "dpso/error.h"
#include "os.h"
#include "windows_utils.h"


const char* dpsoGetCfgPath(const char* appName)
{
    // We use SHGetFolderPath() for XP support; Vista and later has
    // SHGetKnownFolderPath() for this.
    wchar_t appDataPathUtf16[MAX_PATH];
    const auto hresult = SHGetFolderPathW(
            nullptr, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
            nullptr, SHGFP_TYPE_CURRENT, appDataPathUtf16);
    if (FAILED(hresult)) {
        dpsoSetError((
            "SHGetFolderPathW() with CSIDL_LOCAL_APPDATA failed: "
            + dpso::windows::getHresultMessage(hresult)).c_str());
        return nullptr;
    }

    std::wstring pathUtf16 = appDataPathUtf16;
    pathUtf16 += L'\\';
    try {
        pathUtf16 += dpso::windows::utf8ToUtf16(appName);
    } catch (std::runtime_error& e) {
        dpsoSetError((
            std::string{"Can't convert appName to UTF-16: "}
            + e.what()).c_str());
        return nullptr;
    }

    static std::string path;
    try {
        path = dpso::windows::utf16ToUtf8(pathUtf16);
    } catch (std::runtime_error& e) {
        dpsoSetError((
            std::string{"Can't convert path to UTF-8: "}
            + e.what()).c_str());
        return nullptr;
    }

    if (!CreateDirectoryW(pathUtf16.c_str(), nullptr)
            && GetLastError() != ERROR_ALREADY_EXISTS) {
        dpsoSetError((
            "CreateDirectoryW(\"" + path + "\") failed: "
            + dpso::windows::getErrorMessage(GetLastError())
            ).c_str());
        return nullptr;
    }

    return path.c_str();
}
