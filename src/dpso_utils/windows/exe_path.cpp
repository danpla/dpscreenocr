#include "windows/exe_path.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "os_error.h"
#include "windows/error.h"
#include "windows/utf.h"


namespace dpso::windows {


std::string getExePath()
{
    std::wstring path;

    while (true) {
        path.reserve(path.size() + 32);
        path.resize(path.capacity());

        const auto size = GetModuleFileNameW(
            nullptr, path.data(), path.size());

        if (size == 0)
           throw os::Error{
                "GetModuleFileNameW(): "
                + getErrorMessage(GetLastError())};

        if (size < path.size()) {
            path.resize(size);
            break;
        }
    }

    return utf16ToUtf8(path);
}


}
