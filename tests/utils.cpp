#include "utils.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <queue>

#include "flow.h"

#include "dpso_utils/os.h"
#include "dpso_utils/str.h"
#include "dpso_utils/str_stdio.h"
#include "dpso_utils/stream/file_stream.h"
#include "dpso_utils/stream/utils.h"


using namespace dpso;


namespace test::utils {


std::string escapeStr(std::string_view str)
{
    std::string result;
    result.reserve(str.size());

    for (auto c : str)
        switch (c) {
        case 0:
            result += "\\0";
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        case '\\':
            result += "\\\\";
            break;
        default:
            if (std::isprint(static_cast<unsigned char>(c)))
                result += c;
            else
                result +=
                    "\\x"
                    + str::justifyRight(str::toStr(c, 16), 2, '0');
            break;
        }

    return result;
}


std::string toStr(bool b)
{
    return b ? "true" : "false";
}


std::string toStr(const char* str)
{
    return str ? toStr(std::string_view{str}) : "nullptr";
}


std::string toStr(std::string_view str)
{
    return '"' + escapeStr(str) + '"';
}


std::string lfToNativeNewline(std::string_view str)
{
    std::string result;
    result.reserve(str.size());

    for (auto c : str)
        if (c == '\n')
            result += os::newline;
        else
            result += c;

    return result;
}


void saveText(
    std::string_view contextInfo,
    std::string_view filePath,
    std::string_view text)
{
    std::optional<FileStream> file;
    try {
        file.emplace(filePath, FileStream::Mode::write);
    } catch (os::Error& e) {
        test::fatalError(
            "{}: saveText(): FileStream(\"{}\", Mode::write): {}",
            contextInfo, filePath, e.what());
    }

    try {
        write(*file, text);
    } catch (StreamError& e) {
        test::fatalError(
            "{}: saveText(): write(file, ...) to \"{}\": {}",
            contextInfo, filePath, e.what());
    }
}


std::string loadText(
    std::string_view contextInfo, std::string_view filePath)
{
    try {
        return os::loadData(filePath);
    } catch (os::Error& e) {
        test::fatalError(
            "{}: os::loadData(): {}",
            contextInfo, filePath, e.what());
    }
}


static std::string_view extractNextLine(std::string_view& str)
{
    auto pos = std::min(str.find_first_of("\r\n"), str.size());
    if (pos < str.size()) {
        if (str[pos] == '\r'
                && pos + 1 < str.size()
                && str[pos + 1] == '\n')
            ++pos;

        ++pos;
    }

    auto result = str.substr(0, pos);
    str.remove_prefix(pos);
    return result;
}


void printFirstDifference(
    std::string_view expected, std::string_view actual)
{
    const auto maxContextLines = 5;
    std::queue<std::string_view> contextLines;

    while (true) {
        const auto el = extractNextLine(expected);
        const auto al = extractNextLine(actual);

        if (el != al) {
            str::print(
                stderr,
                "First difference between expected (e) and actual "
                "(a) data, with {} preceding\n"
                "line{}. Non-printable characters are escaped with "
                "C-style sequences.\n",
                contextLines.size(),
                contextLines.size() == 1 ? "" : "s");

            while (!contextLines.empty()) {
                str::print(
                    stderr,
                    " |{}\n",
                    escapeStr(contextLines.front()));
                contextLines.pop();
            }

            str::print(stderr, "e|{}\n", escapeStr(el));
            str::print(stderr, "a|{}\n", escapeStr(al));
            break;
        }

        if (el.empty())
            break;

        if (contextLines.size() == maxContextLines)
            contextLines.pop();

        contextLines.push(el);
    }
}


void removeFile(std::string_view filePath)
{
    try {
        os::removeFile(filePath);
    } catch (os::Error& e) {
        test::fatalError(
            "os::removeFile(\"{}\"): {}", filePath, e.what());
    }
}


}
