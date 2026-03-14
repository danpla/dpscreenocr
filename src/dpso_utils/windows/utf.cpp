#include "windows/utf.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "windows/error.h"


namespace dpso::windows {


static int utf8ToUtf16(
    std::string_view utf8Str, wchar_t* dst, int dstSize)
{
    if (utf8Str.empty())
        return 0;

    const auto size = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        utf8Str.data(), utf8Str.size(),
        dst, dstSize);
    if (size <= 0)
        throw CharConversionError(getErrorMessage(GetLastError()));

    return size;
}


std::wstring utf8ToUtf16(std::string_view utf8Str)
{
    std::wstring result(utf8ToUtf16(utf8Str, nullptr, 0), 0);
    utf8ToUtf16(utf8Str, result.data(), result.size());
    return result;
}


static int utf16ToUtf8(
    std::wstring_view utf16Str, char* dst, int dstSize)
{
    if (utf16Str.empty())
        return 0;

    const auto size = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16Str.data(), utf16Str.size(),
        dst, dstSize,
        nullptr, nullptr);
    if (size <= 0)
        throw CharConversionError(getErrorMessage(GetLastError()));

    return size;
}


std::string utf16ToUtf8(std::wstring_view utf16Str)
{
    std::string result(utf16ToUtf8(utf16Str, nullptr, 0), 0);
    utf16ToUtf8(utf16Str, result.data(), result.size());
    return result;
}


}
