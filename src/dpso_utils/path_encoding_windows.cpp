
#include "path_encoding.h"

#include <stdexcept>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "windows/error.h"
#include "windows/utf.h"


namespace dpso {


// Returns the checked result of WideCharToMultiByte(), always > 0.
static int utf16ToAcp(
    const wchar_t* utf16Str, char* dst, int dstSize)
{
    BOOL defaultCharUsed{};

    const auto sizeWithNull = WideCharToMultiByte(
        CP_ACP,
        WC_NO_BEST_FIT_CHARS,
        utf16Str, -1,
        dst, dstSize,
        nullptr, &defaultCharUsed);

    if (sizeWithNull <= 0)
        throw std::runtime_error{
            windows::getErrorMessage(GetLastError())};

    if (defaultCharUsed)
        throw std::runtime_error{
            "UTF-16 string contains characters that cannot be "
            "represented by the current code page (cp"
            + std::to_string(GetACP())
            + ")"};

    return sizeWithNull;
}


static std::string utf16ToAcp(const wchar_t* utf16Str)
{
    const auto sizeWithNull = utf16ToAcp(utf16Str, nullptr, 0);
    std::string result(sizeWithNull - 1, 0);
    utf16ToAcp(utf16Str, result.data(), sizeWithNull);
    return result;
}


std::string convertPathFromUtf8ToSys(const char* utf8Path)
{
    if (GetACP() == CP_UTF8)
        return utf8Path;

    std::wstring utf16Path;

    try {
        utf16Path = windows::utf8ToUtf16(utf8Path);
    } catch (std::runtime_error& e) {
        throw std::runtime_error{
            std::string{"Can't convert path to UTF-16: "} + e.what()};
    }

    return utf16ToAcp(utf16Path.c_str());
}


}
