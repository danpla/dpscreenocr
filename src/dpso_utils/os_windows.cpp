
#include "os.h"

#include <initializer_list>
#include <io.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <fmt/core.h>

#include "windows/cmdline.h"
#include "windows/com.h"
#include "windows/error.h"
#include "windows/handle.h"
#include "windows/utf.h"

#include "str.h"


namespace dpso::os {
namespace {


[[noreturn]]
void throwLastError(const char* description)
{
    const auto lastError = GetLastError();

    const auto message = fmt::format(
        "{}: {}", description, windows::getErrorMessage(lastError));

    switch (lastError) {
    case ERROR_FILE_NOT_FOUND:
        throw FileNotFoundError{message};
    default:
        throw Error{message};
    }
}


std::wstring toUtf16(const char* str, const char* varName)
{
    try {
        return windows::utf8ToUtf16(str);
    } catch (std::runtime_error& e) {
        throw Error{fmt::format(
            "Can't convert {} to UTF-16: {}", varName, e.what())};
    }
}


#define DPSO_WIN_TO_UTF16(VAR_NAME) toUtf16(VAR_NAME, #VAR_NAME)


template<typename T>
bool isDirSep(T c)
{
    return c == '\\' || c == '/';
}


// Skip the root part of a path, which can be a specifier of DOS path
// ("C:\"), a UNC path ("\\server\share\"), or a device path
// ("\\?\C:\", "\\?\\UNC\server\share\"). For details, see:
// https://learn.microsoft.com/en-us/dotnet/standard/io/file-path-formats
// https://ikrima.dev/dev-notes/win-internals/win-path-formats/
//
// We can replace this function with PathCchSkipRoot() from pathcch.h
// once we drop Windows 7.
const char* skipRoot(const char* path)
{
    const auto* p = path;

    if (p[0] && p[1] == ':')
        return p + (isDirSep(p[2]) ? 3 : 2);

    if (isDirSep(p[0]) && isDirSep(p[1])) {
        // UNC or device path.
        p += 2;

        // Device path can look like:
        // \\?\C:\file.txt
        // \\?\UNC\server\share\file.txt
        if (
                p[0] == '?'
                && isDirSep(p[1])
                && str::cmpSubStr(
                    "UNC", p + 2, 3, str::cmpIgnoreCase) == 0
                && isDirSep(p[5]))
            p += 6;

        // Skip "server\share\" of a UNC path, or "?\C:\" of a device
        // path.
        for (auto i = 0; i < 2; ++i) {
            while (*p && !isDirSep(*p))
                ++p;

            if (isDirSep(*p))
                ++p;
        }
    }

    return p;
}


struct PathParts {
    const char* dirBegin;
    const char* dirEnd;
    const char* baseBegin;
    const char* baseEnd;
};


PathParts splitPath(const char* path)
{
    const auto* pathNoRoot = skipRoot(path);

    const auto* sepBegin = pathNoRoot;
    const auto* sepEnd = sepBegin;

    const auto* s = pathNoRoot;

    for (; *s; ++s)
        if (isDirSep(*s)) {
            if (s > pathNoRoot && !isDirSep(s[-1]))
                sepBegin = s;

            sepEnd = s + 1;
        }

    return {
        path, sepBegin == pathNoRoot ? sepEnd : sepBegin, sepEnd, s};
}


}


std::string getErrnoMsg(int errnum)
{
    // On Windows, strerror() does not depend on the locale and is
    // documented to return a pointer to a thread-local storage.
    return std::strerror(errnum);
}


const char* const dirSeparators = "\\/";


std::string getDirName(const char* path)
{
    const auto parts = splitPath(path);
    return {parts.dirBegin, parts.dirEnd};
}


std::string getBaseName(const char* path)
{
    const auto parts = splitPath(path);
    return {parts.baseBegin, parts.baseEnd};
}


// Returns the checked result of WideCharToMultiByte(), always > 0.
static int utf16ToAcp(const wchar_t* utf16Str, char* dst, int dstSize)
{
    BOOL defaultCharUsed{};

    const auto sizeWithNull = WideCharToMultiByte(
        CP_ACP,
        WC_NO_BEST_FIT_CHARS,
        utf16Str, -1,
        dst, dstSize,
        nullptr, &defaultCharUsed);

    if (sizeWithNull <= 0)
        throwLastError("WideCharToMultiByte(CP_ACP, ...)");

    if (defaultCharUsed)
        throw Error{fmt::format(
            "UTF-16 string contains characters that cannot be "
            "represented by the current code page (cp{})",
            GetACP())};

    return sizeWithNull;
}


static std::string utf16ToAcp(const wchar_t* utf16Str)
{
    const auto sizeWithNull = utf16ToAcp(utf16Str, nullptr, 0);
    std::string result(sizeWithNull - 1, 0);
    utf16ToAcp(utf16Str, result.data(), sizeWithNull);
    return result;
}


std::string convertUtf8PathToSys(const char* utf8Path)
{
    if (GetACP() == CP_UTF8)
        return utf8Path;

    return utf16ToAcp(DPSO_WIN_TO_UTF16(utf8Path).c_str());
}


std::int64_t getFileSize(const char* filePath)
{
    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (!GetFileAttributesExW(
            DPSO_WIN_TO_UTF16(filePath).c_str(),
            GetFileExInfoStandard,
            &attrs))
        throwLastError("GetFileAttributesExW()");

    return
        (static_cast<std::uint64_t>(attrs.nFileSizeHigh) << 32)
        | attrs.nFileSizeLow;
}


void resizeFile(const char* filePath, std::int64_t newSize)
{
    windows::Handle<windows::InvalidHandleType::value> file{
        CreateFileW(
            DPSO_WIN_TO_UTF16(filePath).c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr)};

    if (!file)
        throwLastError("CreateFileW()");

    LARGE_INTEGER sizeLi;
    sizeLi.QuadPart = newSize;

    if (!SetFilePointerEx(file, sizeLi, nullptr, FILE_BEGIN))
        throwLastError("SetFilePointerEx()");

    if (!SetEndOfFile(file))
        throwLastError("SetEndOfFile()");
}


FILE* fopen(const char* filePath, const char* mode)
{
    try {
        return _wfopen(
            DPSO_WIN_TO_UTF16(filePath).c_str(),
            DPSO_WIN_TO_UTF16(mode).c_str());
    } catch (std::runtime_error&) {
        errno = EINVAL;
        return nullptr;
    }
}


void removeFile(const char* filePath)
{
    if (!DeleteFileW(DPSO_WIN_TO_UTF16(filePath).c_str()))
        throwLastError("DeleteFileW()");
}


void replace(const char* src, const char* dst)
{
    if (!MoveFileExW(
            DPSO_WIN_TO_UTF16(src).c_str(),
            DPSO_WIN_TO_UTF16(dst).c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throwLastError("MoveFileExW()");
}


void makeDirs(const char* dirPath)
{
    auto dirPathUtf16 = DPSO_WIN_TO_UTF16(dirPath);

    // CreateDirectory() always fails with a permission error instead
    // of ERROR_ALREADY_EXISTS when called for a drive.
    //
    // As a workaround, we first iterate the path backward to
    // calculate the initial part that exists, then forward to create
    // missing directories. This way we also don't have to worry about
    // various path forms ("C:\", "\\?\C:\", "\\?\UNC\LOCALHOST\C$",
    // etc.).
    auto* begin = dirPathUtf16.data();
    auto* s = begin + dirPathUtf16.size();
    while (s > begin) {
        const auto c = *s;
        *s = 0;

        const auto exists =
            GetFileAttributesW(dirPathUtf16.c_str())
                != INVALID_FILE_ATTRIBUTES;

        *s = c;

        if (exists)
            break;

        while (s > begin && isDirSep(s[-1]))
            --s;

        while (s > begin && !isDirSep(s[-1]))
            --s;
    }

    while (*s) {
        while (*s && !isDirSep(*s))
            ++s;

        while (isDirSep(*s))
            ++s;

        const auto c = *s;
        *s = 0;

        if (!CreateDirectoryW(dirPathUtf16.c_str(), nullptr)
                && GetLastError() != ERROR_ALREADY_EXISTS)
            throwLastError("CreateDirectoryW()");

        *s = c;
    }
}


void syncFile(FILE* fp)
{
    const auto fd = _fileno(fp);
    if (fd == -1)
        throwErrno("_fileno()", errno);

    if (_commit(fd) == -1)
        throwErrno("_commit()", errno)};
}


void syncDir(const char* dirPath)
{
    // Windows doesn't support directory synchronization.
    (void)dirPath;
}


static void validateExePath(const char* exePath)
{
    while (str::isSpace(*exePath))
        ++exePath;

    // ShellExecute() opens the current working directory in Explorer
    // if the executable path is empty.
    if (!*exePath)
        throw Error{"Path is empty"};

    const auto* ext = getFileExt(exePath);
    if (!ext)
        return;

    // We can't allow executing batch scripts, because it's impossible
    // to safely pass arbitrary text to them. The ^-escaping rules for
    // variables are broken by design: for example, unescaping happens
    // every time a variable is accessed, even on assignment to
    // another variable.

    // The string can contain trailing whitespace that will be
    // stripped by ShellExecute().
    const auto* extEnd = ext;
    for (const auto* s = ext; *s; ++s)
        if (!str::isSpace(*s))
            extEnd = s + 1;

    for (const auto* batchExt : {".bat", ".cmd"})
        if (str::cmpSubStr(
                batchExt, ext, extEnd - ext, str::cmpIgnoreCase) == 0)
            throw Error{"Execution of batch files is forbidden"};
}


void exec(
    const char* exePath,
    const char* const args[],
    std::size_t numArgs)
{
    validateExePath(exePath);

    const dpso::windows::CoInitializer coInitializer{
        COINIT_APARTMENTTHREADED};

    const auto cmdLine = windows::createCmdLine("", args, numArgs);

    const auto exePathUtf16 = DPSO_WIN_TO_UTF16(exePath);
    const auto cmdLineUtf16 = DPSO_WIN_TO_UTF16(cmdLine.c_str());

    // We use ShellExecute(), as it allows launching a script directly
    // without having to invoke the interpreter explicitly, that is,
    // it does the same as double-click on the file in Explorer. To do
    // the same with CreateProcess(), we need to pass the script path
    // trough cmd.exe, which requires escaping of metacharacters with
    // ^, as well as workarounds for newlines in arguments.
    //
    // For a high-level info about ShellExecute(), see:
    //   https://docs.microsoft.com/en-us/windows/desktop/shell/launch
    SHELLEXECUTEINFOW si{};
    si.cbSize = sizeof(si);
    si.fMask = SEE_MASK_NOCLOSEPROCESS;
    si.lpFile = exePathUtf16.c_str();
    si.lpParameters = cmdLineUtf16.c_str();
    si.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&si))
        throwLastError("ShellExecuteExW()");

    const windows::Handle<windows::InvalidHandleType::null> process{
        si.hProcess};

    // hProcess can be NULL even if ShellExecuteExW() with
    // SEE_MASK_NOCLOSEPROCESS succeeds.
    if (process
            && WaitForSingleObject(process, INFINITE) == WAIT_FAILED)
        throwLastError("WaitForSingleObject()");
}


}
