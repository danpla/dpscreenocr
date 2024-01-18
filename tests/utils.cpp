
#include "utils.h"

#include <cctype>
#include <cerrno>
#include <cstring>
#include <queue>

#include <fmt/core.h>

#include "flow.h"

#include "dpso_utils/os.h"


namespace test::utils {


std::string escapeStr(const char* str)
{
    std::string result;

    while (*str) {
        switch (const auto c = *str++; c) {
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
        default: {
            if (std::isprint(static_cast<unsigned char>(c)))
                result += c;
            else
                result += fmt::format("\\x{:02x}", c);

            break;
        }
        }
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
    const auto* nativeNewline =
        #ifdef _WIN32
        "\r\n";
        #else
        "\n";
        #endif

    std::string result;

    for (const auto* s = str; *s; ++s)
        if (*s == '\n')
            result += nativeNewline;
        else
            result += *s;

    return result;
}


void saveText(
    const char* contextInfo, const char* filePath, const char* text)
{
    dpso::os::StdFileUPtr fp{dpso::os::fopen(filePath, "wb")};
    if (!fp)
        test::fatalError(
            "{}: saveText(): dpsoFopen(\"{}\", \"wb\"): {}\n",
            contextInfo,
            filePath,
            std::strerror(errno));

    if (std::fputs(text, fp.get()) == EOF)
        test::fatalError(
            "{}: saveText(): fputs() to \"{}\" failed\n",
            contextInfo,
            filePath);
}


std::string loadText(const char* contextInfo, const char* filePath)
{
    dpso::os::StdFileUPtr fp{dpso::os::fopen(filePath, "rb")};
    if (!fp)
        test::fatalError(
            "{}: loadText(): dpsoFopen(\"{}\", \"rb\"): {}\n",
            contextInfo,
            filePath,
            std::strerror(errno));

    std::string result;

    char buf[4096];
    while (true) {
        const auto numRead = std::fread(
            buf, 1, sizeof(buf), fp.get());

        result.append(buf, numRead);

        if (numRead < sizeof(buf)) {
            if (std::ferror(fp.get()))
                test::fatalError(
                    "{}: loadText(): fread() from \"{}\" failed\n",
                    contextInfo,
                    filePath);

            break;
        }
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
