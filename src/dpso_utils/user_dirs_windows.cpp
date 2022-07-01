
#include "user_dirs.h"

#include <string>

#include <shlobj.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dpso/backend/windows/utils/error.h"
#include "dpso/error.h"
#include "windows_utils.h"


const char* dpsoGetUserDir(DpsoUserDir userDir, const char* appName)
{
    // FOLDERID_RoamingAppData is actually better path for config, but
    // we keep using FOLDERID_LocalAppData for backward compatibility.
    (void)userDir;

    wchar_t* appDataPathUtf16{};
    const auto hresult = SHGetKnownFolderPath(
            FOLDERID_LocalAppData,
            KF_FLAG_CREATE,
            nullptr,
            &appDataPathUtf16);
    if (FAILED(hresult)) {
        dpsoSetError(
            "SHGetKnownFolderPath() with FOLDERID_LocalAppData "
            "failed: %s",
            dpso::windows::getHresultMessage(hresult).c_str());
        // Docs say that we should call CoTaskMemFree() even if
        // SHGetKnownFolderPath() fails.
        CoTaskMemFree(appDataPathUtf16);
        return nullptr;
    }

    std::wstring pathUtf16 = appDataPathUtf16;
    CoTaskMemFree(appDataPathUtf16);

    pathUtf16 += L'\\';
    try {
        pathUtf16 += dpso::windows::utf8ToUtf16(appName);
    } catch (std::runtime_error& e) {
        dpsoSetError("Can't convert appName to UTF-16: %s", e.what());
        return nullptr;
    }

    static std::string path;
    try {
        path = dpso::windows::utf16ToUtf8(pathUtf16.c_str());
    } catch (std::runtime_error& e) {
        dpsoSetError("Can't convert path to UTF-8: %s", e.what());
        return nullptr;
    }

    if (!CreateDirectoryW(pathUtf16.c_str(), nullptr)
            && GetLastError() != ERROR_ALREADY_EXISTS) {
        dpsoSetError(
            "CreateDirectoryW(\"%s\") failed: %s",
            path.c_str(),
            dpso::windows::getErrorMessage(GetLastError()).c_str());
        return nullptr;
    }

    return path.c_str();
}
