
#include "app_dirs.h"

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dpso_utils/error_set.h"
#include "dpso_utils/windows/error.h"
#include "dpso_utils/windows/utf.h"


// Returns an empty string on error.
static std::wstring getExeDir()
{
    std::wstring result;

    while (true) {
        result.reserve(result.size() + 32);
        result.resize(result.capacity());

        const auto size = GetModuleFileNameW(
            nullptr, result.data(), result.size());

        if (size == 0) {
            dpso::setError(
                "GetModuleFileNameW(): {}",
                dpso::windows::getErrorMessage(GetLastError()));
            return {};
        }

        if (size < result.size()) {
            result.resize(size);
            break;
        }
    }

    const auto slashPos = result.rfind(L'\\');
    if (slashPos != result.npos)
        result.resize(slashPos);

    return result;
}


static std::string baseDirPath;


bool uiInitAppDirs(const char* argv0)
{
    (void)argv0;

    baseDirPath = dpso::windows::utf16ToUtf8(getExeDir().c_str());
    return !baseDirPath.empty();
}


const char* uiGetAppDir(UiAppDir dir)
{
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
