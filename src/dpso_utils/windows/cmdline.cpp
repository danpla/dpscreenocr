#include "windows/cmdline.h"


namespace dpso::windows {


static void appendArgEscaped(std::string& s, std::string_view arg)
{
    // Don't escape if not necessary.
    if (!arg.empty()
            && arg.find_first_of(" \"\r\n\t\v\f") == arg.npos) {
        s += arg;
        return;
    }

    s += '"';

    while (true) {
        const auto pos = arg.find_first_not_of('\\');
        const std::size_t numBackslashes{
            pos == arg.npos ? arg.size() : pos};

        arg.remove_prefix(numBackslashes);

        // Escape quotes and double their preceding backslashes.

        if (arg.empty()) {
            // Escape trailing backslashes.
            s.append(numBackslashes * 2, '\\');
            break;
        }

        if (arg.front() == '"')
            // Escape backslashes and the following quote.
            s.append(numBackslashes * 2 + 1, '\\');
        else
            // Leave backslashes as is.
            s.append(numBackslashes, '\\');

        s += arg.front();
        arg.remove_prefix(1);
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
    std::string_view programName,
    const std::string_view* args, std::size_t numArgs)
{
    std::string result;

    if (programName.find(' ') != programName.npos) {
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
