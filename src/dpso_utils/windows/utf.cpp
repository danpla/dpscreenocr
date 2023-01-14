
#include "windows/utf.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "windows/error.h"


namespace dpso::windows {


std::wstring utf8ToUtf16(const char* utf8Str)
{
    const auto sizeWithNull = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, utf8Str, -1, nullptr, 0);
    if (sizeWithNull <= 0)
        throw std::runtime_error(getErrorMessage(GetLastError()));

    std::wstring result(sizeWithNull - 1, 0);

    if (!MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, utf8Str, -1,
            result.data(), sizeWithNull))
        throw std::runtime_error(getErrorMessage(GetLastError()));

    return result;
}


std::string utf16ToUtf8(const wchar_t* utf16Str)
{
    const auto sizeWithNull = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16Str, -1,
        nullptr, 0,
        nullptr, nullptr);
    if (sizeWithNull <= 0)
        throw std::runtime_error(getErrorMessage(GetLastError()));

    std::string result(sizeWithNull - 1, 0);

    if (!WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS,
            utf16Str, -1,
            result.data(), sizeWithNull,
            nullptr, nullptr))
        throw std::runtime_error(getErrorMessage(GetLastError()));

    return result;
}


}
