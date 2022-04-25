
#include "windows_utils.h"

#include <cstring>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dpso/backend/windows/utils.h"


namespace dpso {
namespace windows {


std::wstring utf8ToUtf16(const char* utf8Str)
{
    const auto sizeWithNull = MultiByteToWideChar(
        CP_UTF8, 0, utf8Str,
        // Tell that the string is null-terminated; the returned size
        // will also include the null.
        -1,
        nullptr, 0);
    if (sizeWithNull <= 0)
        throw std::runtime_error(getErrorMessage(GetLastError()));

    // The C++ standard allows to overwrite string[size()] with 0.
    std::wstring result(sizeWithNull - 1, 0);

    if (!MultiByteToWideChar(
            CP_UTF8, 0,
            utf8Str, -1,
            &result[0], sizeWithNull))
        throw std::runtime_error(getErrorMessage(GetLastError()));

    return result;
}


std::string utf16ToUtf8(const wchar_t* utf16Str)
{
    const auto sizeWithNull = WideCharToMultiByte(
        CP_UTF8, 0,
        utf16Str,
        // Tell that the string is null-terminated; the returned size
        // will also include the null.
        -1,
        nullptr, 0, nullptr, nullptr);
    if (sizeWithNull <= 0)
        throw std::runtime_error(getErrorMessage(GetLastError()));

    // The C++ standard allows to overwrite string[size()] with 0.
    std::string result(sizeWithNull - 1, 0);

    if (!WideCharToMultiByte(
            CP_UTF8, 0,
            utf16Str, -1,
            &result[0], sizeWithNull,
            nullptr, nullptr))
        throw std::runtime_error(getErrorMessage(GetLastError()));

    return result;
}


static void appendArgEscaped(std::string& s, const char* arg)
{
    // Don't escape if not necessary.
    if (*arg && !std::strpbrk(arg, " \"\r\n\t\v\f")) {
        s += arg;
        return;
    }

    s += '"';

    while (true) {
        std::size_t numBackslashes = 0;

        for (; *arg == '\\'; ++arg)
            ++numBackslashes;

        const auto c = *arg++;

        // Escape quotes and double their preceding backslashes.
        if (!c) {
            // Escape trailing backslashes.
            s.append(numBackslashes * 2, '\\');
            break;
        } else if (c == '"')
            // Escape backslashes and the following quote.
            s.append(numBackslashes * 2 + 1, '\\');
        else
            // Leave backslashes as is.
            s.append(numBackslashes, '\\');

        s += c;
    }

    s += '"';
}


// CommandLineToArgvW() doesn't treat the program name like other
// parameters: it can be enclosed in double quotes, but there is no
// special processing for backslashes. For example, if we pass the
// following string to CommandLineToArgvW() as is:
//
//   "a\\\"b" c d
//
// The part within double quotes is parsed as the program name since
// the backslashes are not treated especially. Then the next argument
// (starts with "b") comes immediately after the closing double quote.
// The double quote after "b" enables "in quotes" mode, so all spaces
// till the end of the string are treated as part of the argument
// rather than as separators. CommandLineToArgvW() will thus return 2
// arguments rather than 3: "a\\\" and "b c d".
//
// See:
// * CommandLineToArgv()
// * https://docs.microsoft.com/en-us/cpp/cpp/parsing-cpp-command-line-arguments
// * http://daviddeley.com/autohotkey/parameters/parameters.htm
// * https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/
std::string createCmdLine(
    const char* programName,
    const char* const* args, std::size_t numArgs)
{
    std::string result;

    if (std::strchr(programName, ' ')) {
        result += '"';
        result += programName;
        result += '"';
    } else
        result += programName;

    for (std::size_t i = 0; i < numArgs; ++i) {
        if (!result.empty())
            result += ' ';

        appendArgEscaped(result, args[i]);
    }

    return result;
}


}
}
