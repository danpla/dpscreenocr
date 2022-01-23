
#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>


namespace dpso {
namespace windows {


/**
 * Convert UTF-8 to UTF-16.
 *
 * \throws std::runtime_error
 */
std::wstring utf8ToUtf16(const char* utf8Str);


/**
 * Convert UTF-16 to UTF-8.
 *
 * \throws std::runtime_error
 */
std::string utf16ToUtf8(const wchar_t* utf16Str);


/**
 * Create command line using CommandLineToArgv() rules.
 *
 * Please note that on Windows the program name is parsed differently
 * from the other parameters, and therefore comes as a separate
 * programName argument in this function. If you need to build the
 * command-line arguments without the program name (for example, to
 * be used as lpCommandLine for CreateProcess() when lpApplicationName
 * is not null), leave programName empty.
 */
std::string createCmdLine(
    const char* programName,
    const char* const* args, std::size_t numArgs);

template<std::size_t N>
std::string createCmdLine(
    const char* programName, const char* const (&args)[N])
{
    return createCmdLine(programName, args, N);
}


}
}
