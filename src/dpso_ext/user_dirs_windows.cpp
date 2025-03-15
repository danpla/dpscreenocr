#include "user_dirs.h"

#include <string>

#include <shlobj.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dpso_utils/error_set.h"
#include "dpso_utils/scope_exit.h"
#include "dpso_utils/windows/error.h"
#include "dpso_utils/windows/utf.h"


using namespace dpso;


const char* dpsoGetUserDir(DpsoUserDir userDir, const char* appName)
{
    // FOLDERID_RoamingAppData is actually better path for config, but
    // we keep using FOLDERID_LocalAppData for backward compatibility.
    (void)userDir;

    wchar_t* appDataPathUtf16{};
    // Docs say that we should call CoTaskMemFree() even if
    // SHGetKnownFolderPath() fails.
    const ScopeExit taskMemFree{
        [&]{ CoTaskMemFree(appDataPathUtf16); }};

    const auto hresult = SHGetKnownFolderPath(
        FOLDERID_LocalAppData,
        KF_FLAG_DONT_VERIFY,
        nullptr,
        &appDataPathUtf16);
    if (FAILED(hresult)) {
        setError(
            "SHGetKnownFolderPath(FOLDERID_LocalAppData, ...): {}",
            windows::getHresultMessage(hresult));
        return {};
    }

    static std::string path;
    try {
        path = windows::utf16ToUtf8(appDataPathUtf16);
    } catch (windows::CharConversionError& e) {
        setError("Can't convert path to UTF-8: {}", e.what());
        return {};
    }

    path += '\\';
    path += appName;

    return path.c_str();
}
