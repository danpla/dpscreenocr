
#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>


namespace dpso {
namespace win {


/**
 * Convert UTF-8 to UTF-16.
 *
 * \throws std::runtime_error
 */
std::wstring utf8ToUtf16(const char* utf8Str);

inline std::wstring utf8ToUtf16(const std::string& utf8Str)
{
    return utf8ToUtf16(utf8Str.c_str());
}


/**
 * Convert UTF-16 to UTF-8.
 *
 * \throws std::runtime_error
 */
std::string utf16ToUtf8(const wchar_t* utf16Str);

inline std::string utf16ToUtf8(const std::wstring& utf16Str)
{
    return utf16ToUtf8(utf16Str.c_str());
}


/**
 * Convert arguments to command line using CommandLineToArgv() rules.
 */
std::string argvToCmdLine(const char** argv, std::size_t argc);

template<std::size_t N>
std::string argvToCmdLine(const char* (&argv)[N])
{
    return argvToCmdLine(argv, N);
}


}
}
