
#include "windows_utils.h"

#include <windows.h>


namespace dpso {
namespace win {


// TODO: This is temporary; we should import that function from
// dpso lib.
static std::string getLastErrorMessage()
{
    const auto error = GetLastError();
    if (error == ERROR_SUCCESS)
        return "";

    char* messageBuffer = nullptr;
    const auto size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        reinterpret_cast<char*>(&messageBuffer),
        0, nullptr);

    if (size == 0)
        return "Windows error " + std::to_string(error);

    std::string message(messageBuffer, size);

    LocalFree(messageBuffer);

    return message;
}


std::wstring utf8ToUtf16(const char* utf8Str)
{
    if (!*utf8Str)
        return {};

    const auto sizeRequired = MultiByteToWideChar(
        CP_UTF8, 0, utf8Str,
        // Tell that the string is null-terminated; the returned size
        // will also include the null.
        -1,
        nullptr, 0);
    if (sizeRequired <= 0)
        throw std::runtime_error(getLastErrorMessage());

    std::wstring result;
    result.resize(sizeRequired - 1);

    if (!MultiByteToWideChar(
            CP_UTF8, 0,
            utf8Str, -1,
            // Note that we overwrite result[size()] with CharT(),
            // which is allowed by the C++ standard.
            &result[0], sizeRequired))
        throw std::runtime_error(getLastErrorMessage());

    return result;
}


std::string utf16ToUtf8(const wchar_t* utf16Str)
{
    if (!*utf16Str)
        return {};

    const auto sizeRequired = WideCharToMultiByte(
        CP_UTF8, 0,
        utf16Str,
        // Tell that the string is null-terminated; the returned size
        // will also include the null.
        -1,
        nullptr, 0, nullptr, nullptr);
    if (sizeRequired <= 0)
        throw std::runtime_error(getLastErrorMessage());

    std::string result;
    result.resize(sizeRequired - 1);

    if (!WideCharToMultiByte(
            CP_UTF8, 0,
            utf16Str, -1,
            // Note that we overwrite result[size()] with CharT(),
            // which is allowed by the C++ standard.
            &result[0], sizeRequired,
            nullptr, nullptr))
        throw std::runtime_error(getLastErrorMessage());

    return result;
}

}
}
