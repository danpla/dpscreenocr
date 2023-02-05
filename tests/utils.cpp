
#include "utils.h"

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <queue>

#include "flow.h"

#include "dpso_utils/os.h"


namespace test {
namespace utils {


std::string boolToStr(bool b)
{
    return b ? "true" : "false";
}


std::string escapeStr(const char* str)
{
    std::string result;

    while (*str) {
        const auto c = *str++;

        switch (c) {
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
            if (std::isprint(c))
                result += c;
            else {
                char buf[5];
                std::snprintf(buf, sizeof(buf), "\\x%02hhx", c);
                result += buf;
            }
            break;
        }
        }
    }

    return result;
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
    dpso::StdFileUPtr fp{dpsoFopen(filePath, "wb")};
    if (!fp)
        test::fatalError(
            "%s: saveText(): dpsoFopen(\"%s\", \"wb\"): %s\n",
            contextInfo,
            filePath,
            std::strerror(errno));

    if (std::fputs(text, fp.get()) == EOF)
        test::fatalError(
            "%s: saveText(): fputs() to \"%s\" failed\n",
            contextInfo,
            filePath);
}


std::string loadText(const char* contextInfo, const char* filePath)
{
    dpso::StdFileUPtr fp{dpsoFopen(filePath, "rb")};
    if (!fp)
        test::fatalError(
            "%s: loadText(): dpsoFopen(\"%s\", \"rb\"): %s\n",
            contextInfo,
            filePath,
            std::strerror(errno));

    std::string result;

    char buf[4096];
    while (true) {
        const auto numRead = std::fread(
            buf, 1, sizeof(buf), fp.get());
        if (numRead == 0) {
            if (std::ferror(fp.get()))
                test::fatalError(
                    "%s: loadText(): fread() from \"%s\" failed\n",
                    contextInfo,
                    filePath);

            break;
        }

        result.append(buf, numRead);
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
                    escapeStr(contextLines.front().c_str()).c_str());
                contextLines.pop();
            }

            std::fprintf(
                stderr, "e|%s\n", escapeStr(el.c_str()).c_str());
            std::fprintf(
                stderr, "a|%s\n", escapeStr(al.c_str()).c_str());
            break;
        }

        if (el.empty())
            break;

        if (contextLines.size() == maxContextLines)
            contextLines.pop();

        contextLines.push(el);
    }
}


}
}
