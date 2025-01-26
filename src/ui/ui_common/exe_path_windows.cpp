#include "exe_path.h"

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dpso_utils/error_set.h"
#include "dpso_utils/windows/error.h"
#include "dpso_utils/windows/utf.h"


namespace ui {


static std::string exePath;


bool initExePath(const char* argv0)
{
    (void)argv0;

    std::wstring path;

    while (true) {
        path.reserve(path.size() + 32);
        path.resize(path.capacity());

        const auto size = GetModuleFileNameW(
            nullptr, path.data(), path.size());

        if (size == 0) {
            dpso::setError(
                "GetModuleFileNameW(): {}",
                dpso::windows::getErrorMessage(GetLastError()));
            return false;
        }

        if (size < path.size()) {
            path.resize(size);
            break;
        }
    }

    exePath = dpso::windows::utf16ToUtf8(path.c_str());
    return true;
}


const std::string& getToplevelExePath()
{
    return exePath;
}


const std::string& getExePath()
{
    return exePath;
}


}
