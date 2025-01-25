
#include "utils.h"

#include <cctype>
#include <cstring>
#include <optional>
#include <queue>

#include "flow.h"

#include "dpso_utils/os.h"
#include "dpso_utils/str.h"
#include "dpso_utils/stream/file_stream.h"
#include "dpso_utils/stream/utils.h"


namespace test::utils {


// escapeStr() that takes the length so that we can include embedded
// nulls from std::string.
static std::string escapeStr(const char* str, std::size_t strLen)
{
    std::string result;
    result.reserve(strLen);

    for (std::size_t i = 0; i < strLen; ++i)
        switch (const auto c = str[i]) {
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
                    + dpso::str::rightJustify(
                        dpso::str::toStr(c, 16),
                        2,
                        '0');
            break;
        }

    return result;
}


std::string escapeStr(const char* str)
{
    return escapeStr(str, std::strlen(str));
}


std::string escapeStr(const std::string& str)
{
    return escapeStr(str.c_str(), str.size());
}


std::string toStr(bool b)
{
    return b ? "true" : "false";
}


std::string toStr(const char* str)
{
    if (!str)
        return "nullptr";

    return '"' + escapeStr(str) + '"';
}


std::string toStr(const std::string& str)
{
    return '"' + escapeStr(str) + '"';
}


std::string lfToNativeNewline(const char* str)
{
    std::string result;

    for (const auto* s = str; *s; ++s)
        if (*s == '\n')
            result += dpso::os::newline;
        else
            result += *s;

    return result;
}


void saveText(
    const char* contextInfo, const char* filePath, const char* text)
{
    std::optional<dpso::FileStream> file;
    try {
        file.emplace(filePath, dpso::FileStream::Mode::write);
    } catch (dpso::os::Error& e) {
        test::fatalError(
            "{}: saveText(): FileStream(\"{}\", Mode::write): {}",
            contextInfo,
            filePath,
            e.what());
    }

    try {
        dpso::write(*file, text);
    } catch (dpso::StreamError& e) {
        test::fatalError(
            "{}: saveText(): write(file, ...) to \"{}\": {}",
            contextInfo,
            filePath,
            e.what());
    }
}


std::string loadText(const char* contextInfo, const char* filePath)
{
    std::optional<dpso::FileStream> file;
    try {
        file.emplace(filePath, dpso::FileStream::Mode::read);
    } catch (dpso::os::Error& e) {
        test::fatalError(
            "{}: loadText(): FileStream(\"{}\", Mode::read): {}",
            contextInfo,
            filePath,
            e.what());
    }

    std::string result;

    char buf[4096];
    while (true) {
        std::size_t numRead{};
        try {
            numRead = file->readSome(buf, sizeof(buf));
        } catch (dpso::StreamError& e) {
            test::fatalError(
                "{}: loadText(): FileStream::readSome() from \"{}\": "
                "{}",
                contextInfo,
                filePath,
                e.what());
        }

        result.append(buf, numRead);

        if (numRead < sizeof(buf))
            break;
    }

    return result;
}


static std::string getNextLine(const char*& str)
{
    const auto* lineBegin = str;

    for (; *str; ++str) {
        if (*str == '\r' || *str == '\n') {
            if (*str == '\r' && str[1] == '\n')
                ++str;

            ++str;
            break;
        }
    }

    return {lineBegin, str};
}


void printFirstDifference(const char* expected, const char* actual)
{
    const auto maxContextLines = 5;
    std::queue<std::string> contextLines;

    const auto* e = expected;
    const auto* a = actual;
    while (true) {
        const auto el = getNextLine(e);
        const auto al = getNextLine(a);

        if (el != al) {
            std::fprintf(
                stderr,
                "First difference between expected (e) and actual "
                "(a) data, with %zu preceding\n"
                "line%s. Non-printable characters are escaped with "
                "C-style sequences.\n",
                contextLines.size(),
                contextLines.size() == 1 ? "" : "s");

            while (!contextLines.empty()) {
                std::fprintf(
                    stderr,
                    " |%s\n",
                    escapeStr(contextLines.front()).c_str());
                contextLines.pop();
            }

            std::fprintf(stderr, "e|%s\n", escapeStr(el).c_str());
            std::fprintf(stderr, "a|%s\n", escapeStr(al).c_str());
            break;
        }

        if (el.empty())
            break;

        if (contextLines.size() == maxContextLines)
            contextLines.pop();

        contextLines.push(el);
    }
}


void removeFile(const char* filePath)
{
    try {
        dpso::os::removeFile(filePath);
    } catch (dpso::os::FileNotFoundError&) {
    } catch (dpso::os::Error& e) {
        test::fatalError(
            "os::removeFile(\"{}\"): {}", filePath, e.what());
    }
}


}
