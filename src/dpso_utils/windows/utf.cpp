#include "windows/utf.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "windows/error.h"


namespace dpso::windows {


static int utf8ToUtf16(const char* utf8Str, wchar_t* dst, int dstSize)
{
    const auto sizeWithNull = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, utf8Str, -1, dst, dstSize);
    if (sizeWithNull <= 0)
        throw CharConversionError(getErrorMessage(GetLastError()));

    return sizeWithNull;
}


std::wstring utf8ToUtf16(const char* utf8Str)
{
    const auto sizeWithNull = utf8ToUtf16(utf8Str, nullptr, 0);
    std::wstring result(sizeWithNull - 1, 0);
    utf8ToUtf16(utf8Str, result.data(), sizeWithNull);
    return result;
}


static int utf16ToUtf8(
    const wchar_t* utf16Str, char* dst, int dstSize)
{
    const auto sizeWithNull = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16Str, -1,
        dst, dstSize,
        nullptr, nullptr);
    if (sizeWithNull <= 0)
        throw CharConversionError(getErrorMessage(GetLastError()));

    return sizeWithNull;
}


std::string utf16ToUtf8(const wchar_t* utf16Str)
{
    const auto sizeWithNull = utf16ToUtf8(utf16Str, nullptr, 0);
    std::string result(sizeWithNull - 1, 0);
    utf16ToUtf8(utf16Str, result.data(), sizeWithNull);
    return result;
}


}
