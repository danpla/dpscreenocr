
#include "utils.h"

#include <cctype>
#include <optional>
#include <queue>

#include <fmt/core.h>

#include "flow.h"

#include "dpso_utils/os.h"
#include "dpso_utils/stream/file_stream.h"
#include "dpso_utils/stream/utils.h"


namespace test::utils {


std::string escapeStr(const char* str)
{
    std::string result;

    while (*str)
        switch (const auto c = *str++) {
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
                result += fmt::format("\\x{:02x}", c);
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
    if (!str)
        return "nullptr";

    return '"' + escapeStr(str) + '"';
}


std::string toStr(const std::string& str)
{
    return toStr(str.c_str());
}


std::string lfToNativeNewline(const char* str)
{
    std::string result;

    for (const auto* s = str; *s; ++s)
        if (*s == '\n')
            result += DPSO_OS_NEWLINE;
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
            "{}: saveText(): FileStream(\"{}\", Mode::write): {}\n",
            contextInfo,
            filePath,
            e.what());
    }

    try {
        dpso::write(*file, text);
    } catch (dpso::StreamError& e) {
        test::fatalError(
            "{}: saveText(): write(file, ...) to \"{}\": {}\n",
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
            "{}: loadText(): FileStream(\"{}\", Mode::read): {}\n",
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
                "{}\n",
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
            fmt::print(
                stderr,
                "First difference between expected (e) and actual "
                "(a) data, with {} preceding\n"
                "line{}. Non-printable characters are escaped with "
                "C-style sequences.\n",
                contextLines.size(),
                contextLines.size() == 1 ? "" : "s");

            while (!contextLines.empty()) {
                fmt::print(
                    stderr,
                    " |{}\n",
                    escapeStr(contextLines.front().c_str()));
                contextLines.pop();
            }

            fmt::print(stderr, "e|{}\n", escapeStr(el.c_str()));
            fmt::print(stderr, "a|{}\n", escapeStr(al.c_str()));
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
