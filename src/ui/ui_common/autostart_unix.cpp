#include "autostart.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <sys/stat.h>

#include "dpso_utils/error_set.h"
#include "dpso_utils/line_reader.h"
#include "dpso_utils/os.h"
#include "dpso_utils/str.h"
#include "dpso_utils/stream/file_stream.h"
#include "dpso_utils/stream/utils.h"
#include "dpso_utils/unix/path_env_search.h"
#include "dpso_utils/unix/xdg_dirs.h"


using namespace dpso;


// Specifications:
// * Desktop Entry Specification
//   https://specifications.freedesktop.org/desktop-entry-spec/latest/
// * Desktop Application Autostart Specification
//   https://specifications.freedesktop.org/autostart-spec/latest/
//
// To validate a desktop file, use desktop-file-validate from the
// desktop-file-utils package.


namespace {


class AutostartError : public std::runtime_error {
    using runtime_error::runtime_error;
};


// Escape the "string" type defined by the desktop entry spec.
std::string escape(const char* str)
{
    std::string result;

    for (const auto* s = str; *s; ++s) {
        // Escaping spaces and tabs is only necessary if it's a
        // leading or trailing character, to avoid trimming during
        // parsing.
        if ((*s == ' ' || *s == '\t') && (s > str && s[1])) {
            result += *s;
            continue;
        }

        switch (*s) {
        case ' ':
            result += "\\s";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\t':
            result += "\\t";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\\':
            result += "\\\\";
            break;
        default:
            result += *s;
            break;
        }
    }

    return result;
}


// Appends an unescaped form of the given escaped character of the
// "string" type defined by the desktop entry spec.
void appendUnescaped(std::string& str, char c)
{
    switch (c) {
    case 's':
        str += ' ';
        break;
    case 'n':
        str += '\n';
        break;
    case 't':
        str += '\t';
        break;
    case 'r':
        str += '\r';
        break;
    case '\\':
        str += '\\';
        break;
    default:
        str += '\\';
        str += c;
        break;
    }
}


// Unescape the "string" type defined by the desktop entry spec.
std::string unescape(const char* str)
{
    std::string result;

    while (*str)
        if (const auto c = *str++; c != '\\')
            result += c;
        else if (*str)
            appendUnescaped(result, *str++);

    return result;
}


// Return true if c is a reserved character of an argument, and thus
// requires the argument to be quoted.
bool isReservedArgChar(char c)
{
    return std::strchr(" \t\n\"'\\><~|&;$*?#()`", c);
}


bool needsQuoting(const char* arg)
{
    if (!*arg)
        return true;

    for (; *arg; ++arg)
        if (isReservedArgChar(*arg))
            return true;

    return false;
}


// Return true if the character must be escaped inside a quoted
// argument.
bool isEscapableArgChar(char c)
{
    return std::strchr("\"`$\\", c);
}


void appendArg(std::string& cmdLine, const char* arg)
{
    if (!cmdLine.empty())
        cmdLine += ' ';

    if (!needsQuoting(arg)) {
        cmdLine += arg;
        return;
    }

    cmdLine += '"';

    for (; *arg; ++arg) {
        if (isEscapableArgChar(*arg))
            cmdLine += '\\';

        cmdLine += *arg;
    }

    cmdLine += '"';
}


// Extracts the next argument from the Exec string. When the function
// returns, the exec string will point past the last parsed character.
// Returns nullopt if no arguments left. Throws AutostartError if
// the argument is incorrectly quoted.
std::optional<std::string> getNextArg(const char*& cmdLine)
{
    while (str::isSpace(*cmdLine))
        ++cmdLine;

    if (!*cmdLine)
        return {};

    if (*cmdLine != '"') {
        const auto* argBegin = cmdLine;
        while (*cmdLine && *cmdLine != ' ') {
            if (isReservedArgChar(*cmdLine))
                throw AutostartError{str::format(
                    "Reserved character \"{}\" outside a quoted "
                    "argument",
                    *cmdLine)};

            ++cmdLine;
        }

        return std::string{argBegin, cmdLine};
    }

    // Quoted argument.

    // Skip the leading double quote.
    ++cmdLine;

    std::string result;
    while (true) {
        if (!*cmdLine)
            throw AutostartError{
                "Quoted argument has no closing \""};

        if (const auto c = *cmdLine++; c == '"')
            break;
        else if (c != '\\') {
            if (isEscapableArgChar(c))
                throw AutostartError{str::format(
                    "Unescaped \"{}\" inside a quoted argument", c)};

            result += c;
            continue;
        }

        if (!*cmdLine)
            throw AutostartError{"Command line ends with \\"};

        const auto c = *cmdLine++;
        if (!isEscapableArgChar(c))
            result += '\\';

        result += c;
    }

    return result;
}


// Check if the value is inside the string list as defined by the
// "string(s)" type of the desktop entry spec.
bool isInStrings(const char* strings, const char* val)
{
    if (!*strings)
        return false;

    const auto sep = ';';

    const auto* s = strings;
    std::string str;
    while (true) {
        if (!*s || *s == sep) {
            if (str == val)
                return true;

            str.clear();

            // A trailing separator is optional for the last value in
            // the list, unless it's an empty string.
            if (*s == sep)
                ++s;

            if (!*s)
                break;
        }

        if (const auto c = *s++; c != '\\') {
            str += c;
            continue;
        }

        if (!*s)
            continue;

        // Strings in lists support an extra escape sequence for ;.
        if (const auto c = *s++; c == sep)
            str += sep;
        else
            appendUnescaped(str, c);
    }

    return false;
}


// Check if the given string list contains one of the desktop names
// from $XDG_CURRENT_DESKTOP.
bool containsCurrentDesktop(const char* strings)
{
    const auto* s = std::getenv("XDG_CURRENT_DESKTOP");
    if (!s)
        return false;

    const auto sep = ':';

    std::string desktop;
    while (*s) {
        const auto* begin = s;
        const auto* end = begin;

        while (*end && *end != sep)
            ++end;

        desktop.assign(begin, end);
        if (!desktop.empty() && isInStrings(strings, desktop.c_str()))
            return true;

        if (*end == sep)
            ++end;

        s = end;
    }

    return false;
}


struct DesktopFileInfo {
    std::string cmdLine;
    bool isEnabled;
};


// Returns nullopt if the desktop file doesn't exist or has a
// "Hidden=true" line. Throws AutostartError on errors.
std::optional<DesktopFileInfo> getDesktopFileInfo(
    const char* desktopFilePath)
{
    std::optional<FileStream> file;
    try {
        file.emplace(desktopFilePath, FileStream::Mode::read);
    } catch (os::FileNotFoundError&) {
        return {};
    } catch (os::Error& e) {
        throw AutostartError{str::format(
            "FileStream(..., Mode::read): {}", e.what())};
    }

    // Desktop entries are enabled by default, unless the OnlyShowIn
    // key is present.
    DesktopFileInfo result{{}, true};

    LineReader lineReader{*file};
    std::string line;
    bool wasHeader{};
    while (true) {
        try {
            if (!lineReader.readLine(line))
                break;
        } catch (StreamError& e) {
            throw AutostartError{str::format(
                "LineReader::readLine(): {}", e.what())};
        }

        if (line.empty() || line.front() == '#')
            continue;

        if (!wasHeader) {
            const auto* mainGroup = "[Desktop Entry]";
            if (line == mainGroup) {
                wasHeader = true;
                continue;
            }

            // The spec says that there should be nothing preceding
            // [Desktop Entry] but possibly one or more comments.
            throw AutostartError{str::format(
                "Unexpected line \"{}\" before {}", line, mainGroup)};
        }

        if (line.front() == '[')
            // Another group.
            break;

        const auto equalsPos = line.find('=');
        if (equalsPos == line.npos)
            throw AutostartError{str::format(
                "No = in \"{}\"", line)};

        // The spec says that space around the = sign should be
        // ignored, but doesn't mention space before the key or
        // after the value.

        auto keyEnd = equalsPos;
        while (keyEnd > 0 && str::isSpace(line[keyEnd - 1]))
            --keyEnd;

        if (keyEnd == 0)
            throw AutostartError{str::format(
                "Key is empty in \"{}\"", line)};

        line[keyEnd] = 0;
        const auto* key = line.c_str();

        const auto* val = line.c_str() + equalsPos + 1;
        while (str::isSpace(*val))
            ++val;

        if (std::strcmp(key, "Exec") == 0)
            result.cmdLine = unescape(val);
        else if (std::strcmp(key, "Hidden") == 0
                && std::strcmp(val, "true") == 0)
            // The spec says that "Hidden=true" is strictly equivalent
            // to the .desktop file not existing at all.
            return {};
        else if (std::strcmp(key, "OnlyShowIn") == 0)
            result.isEnabled = containsCurrentDesktop(val);
        else if (std::strcmp(key, "NotShowIn") == 0)
            result.isEnabled = !containsCurrentDesktop(val);
    }

    return result;
}


bool isSameFsEntry(const char* aPath, const char* bPath)
{
    struct stat a, b;
    return
        stat(aPath, &a) == 0
        && stat(bPath, &b) == 0
        && a.st_dev == b.st_dev
        && a.st_ino == b.st_ino;
}


std::string getAltDesktopFileName(const char* appName)
{
    std::string result{appName};
    // This is the conversion done by the XFCE autostart manager.
    std::replace(result.begin(), result.end(), '/', '-');
    return result;
}


}


struct UiAutostart {
    std::string appName;
    std::string cmdLine;
    std::string desktopFilePath;
    bool isEnabled;
};


static UiAutostart* create(const UiAutostartArgs* args)
{
    if (!args)
        throw AutostartError{"args is null"};

    if (args->numArgs == 0)
        throw AutostartError{"args->numArgs is 0"};

    if (*args->args[0] != '/')
        throw AutostartError{
            "args->args[0] must be an absolute path"};

    std::string autostartDir;
    try {
        autostartDir =
            unix::getXdgDirPath(unix::XdgDir::configHome)
            + "/autostart";
    } catch (os::Error& e) {
        throw AutostartError{str::format(
            "unix::getXdgDirPath(XdgDir::configHome): {}", e.what())};
    }

    auto primaryDesktopFilePath = str::format(
        "{}/{}.desktop", autostartDir, args->appFileName);

    // The alternative name is the one most likely to be used by
    // system autostart managers: on most desktops, their dialogs ask
    // the user for a program name, and the desktop file name is
    // automatically derived from that.
    auto altDesktopFilePath = str::format(
        "{}/{}.desktop",
        autostartDir, getAltDesktopFileName(args->appName));

    auto* desktopFilePath = &primaryDesktopFilePath;
    bool isEnabled{};

    // Try the primary name first, as this is what we use by default
    // if there is no existing desktop entry. But if the file with the
    // alt name matches, there's no harm in continuing to use it.
    for (auto* path
            : {&primaryDesktopFilePath, &altDesktopFilePath}) {
        std::optional<DesktopFileInfo> desktopFileInfo;
        try {
            desktopFileInfo = getDesktopFileInfo(path->c_str());
        } catch (AutostartError& e) {
            throw AutostartError{str::format(
                "Can't load \"{}\": {}", *path, e.what())};
        }

        if (!desktopFileInfo)
            continue;

        std::string desktopFileExe;
        try {
            const char* cmdLine = desktopFileInfo->cmdLine.c_str();
            desktopFileExe = getNextArg(cmdLine).value_or("");
        } catch (AutostartError& e) {
            throw AutostartError{str::format(
                "Invalid Exec command line \"{}\" in \"{}\": {}",
                desktopFileInfo->cmdLine, *path, e.what())};
        }

        if (desktopFileExe.empty())
            continue;

        // The executable from Exec should be specified either as an
        // absolute path or just a name to be searched in $PATH.
        if (const auto slashPos = desktopFileExe.find('/');
                slashPos == desktopFileExe.npos) {
            desktopFileExe = unix::findInPathEnv(
                desktopFileExe.c_str());
            if (desktopFileExe.empty())
                continue;
        } else if (slashPos != 0)
            throw AutostartError{str::format(
                "Exec command line (\"{}\") in \"{}\" cannot have a "
                "relative executable path",
                desktopFileInfo->cmdLine, *path)};

        // Do the string comparison first as an optimization for the
        // common case when both paths are already canonical.
        if (args->args[0] == desktopFileExe
                || isSameFsEntry(
                    args->args[0], desktopFileExe.c_str())) {
            desktopFilePath = path;
            isEnabled = desktopFileInfo->isEnabled;
            break;
        }
    }

    std::string cmdLine;
    for (std::size_t i = 0; i < args->numArgs; ++i)
        appendArg(cmdLine, args->args[i]);

    return new UiAutostart{
        args->appName,
        std::move(cmdLine),
        std::move(*desktopFilePath),
        isEnabled};
}


UiAutostart* uiAutostartCreate(const UiAutostartArgs* args)
{
    try {
        return create(args);
    } catch (AutostartError& e) {
        setError("{}", e.what());
        return {};
    }
}


void uiAutostartDelete(UiAutostart* autostart)
{
    delete autostart;
}


bool uiAutostartGetIsEnabled(const UiAutostart* autostart)
{
    return autostart && autostart->isEnabled;
}


static void setIsEnabled(UiAutostart* autostart, bool newIsEnabled)
{
    if (!autostart)
        throw AutostartError{"autostart is null"};

    if (newIsEnabled == autostart->isEnabled)
        return;

    if (!newIsEnabled) {
        try {
            os::removeFile(autostart->desktopFilePath.c_str());
        } catch (os::FileNotFoundError&) {
        } catch (os::Error& e) {
            throw AutostartError{str::format(
                "os::removeFile(\"{}\"): {}",
                autostart->desktopFilePath, e.what())};
        }

        autostart->isEnabled = false;
        return;
    }

    const auto desktopFileDir = os::getDirName(
        autostart->desktopFilePath.c_str());
    try {
        os::makeDirs(desktopFileDir.c_str());
    } catch (os::Error& e) {
        throw AutostartError{str::format(
            "os::makeDirs(\"{}\"): {}", desktopFileDir, e.what())};
    }

    std::optional<FileStream> file;
    try {
        file.emplace(
            autostart->desktopFilePath.c_str(),
            FileStream::Mode::write);
    } catch (os::Error& e) {
        throw AutostartError{str::format(
            "FileStream(\"{}\", Mode::write): {}",
            autostart->desktopFilePath, e.what())};
    }

    try {
        write(
            *file,
            str::format(
                "[Desktop Entry]\n"
                "Type=Application\n"
                "Name={}\n"
                "Exec={}\n",
                escape(autostart->appName.c_str()),
                escape(autostart->cmdLine.c_str())));
    } catch (StreamError& e) {
        try {
            os::removeFile(autostart->desktopFilePath.c_str());
        } catch (os::Error&) {
        }

        throw AutostartError{str::format(
            "write(file, ...) to \"{}\": {}",
            autostart->desktopFilePath, e.what())};
    }

    autostart->isEnabled = true;
}


bool uiAutostartSetIsEnabled(
    UiAutostart* autostart, bool newIsEnabled)
{
    try {
        setIsEnabled(autostart, newIsEnabled);
        return true;
    } catch (AutostartError& e) {
        setError("{}", e.what());
        return false;
    }
}
