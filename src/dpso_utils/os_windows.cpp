
#include "os.h"

#include <cerrno>
#include <cstring>
#include <io.h>

#include "windows/error.h"
#include "windows/utf.h"


namespace dpso::os {


[[noreturn]]
static void throwLastError(const char* description)
{
    const auto lastError = GetLastError();

    const auto message =
        std::string{description}
        + ": "
        + windows::getErrorMessage(lastError);

    switch (lastError) {
    case ERROR_FILE_NOT_FOUND:
        throw FileNotFoundError{message};
    default:
        throw Error{message};
    }
}


static std::wstring toUtf16(const char* str, const char* varName)
{
    try {
        return windows::utf8ToUtf16(str);
    } catch (std::runtime_error& e) {
        throw Error{
            std::string{"Can't convert "}
            + varName
            + " to UTF-16: "
            + e.what()};
    }
}


#define DPSO_WIN_TO_UTF16(VAR_NAME) toUtf16(VAR_NAME, #VAR_NAME)


const char* const dirSeparators = "\\/";


std::string getDirName(const char* path)
{
    static_assert(false, "TODO");
}


std::string getBaseName(const char* path)
{
    static_assert(false, "TODO");
}


std::int64_t getFileSize(const char* filePath)
{
    const auto filePathUtf16 = DPSO_WIN_TO_UTF16(filePath);

    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (!GetFileAttributesExW(
            filePathUtf16.c_str(), GetFileExInfoStandard, &attrs))
        throwLastError("GetFileAttributesExW()");

    return
        (static_cast<std::uint64_t>(attrs.nFileSizeHigh) << 32)
        | attrs.nFileSizeLow;
}


FILE* fopen(const char* filePath, const char* mode)
{
    std::wstring filePathUtf16;
    std::wstring modeUtf16;

    try {
        filePathUtf16 = windows::utf8ToUtf16(filePath);
        modeUtf16 = windows::utf8ToUtf16(mode);
    } catch (std::runtime_error&) {
        errno = EINVAL;
        return nullptr;
    }

    return _wfopen(filePathUtf16.c_str(), modeUtf16.c_str());
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


static bool isDirSep(wchar_t c)
{
    return c == L'\\' || c == L'/';
}


void makeDirs(const char* dirPath)
{
    auto dirPathUtf16 = DPSO_WIN_TO_UTF16(dirPath);

    // CreateDirectory() always fails with a permission error instead
    // of ERROR_ALREADY_EXISTS when called for a drive.
    //
    // We could skip the drive, but PathCchSkipRoot() is available
    // since Windows 8 (we need to support 7), and we don't want to
    // write our own routine for this, since we have to deal with all
    // kinds of path forms ("C:\", "\\?\C:\", "\\?\UNC\LOCALHOST\C$",
    // etc.). Instead, we first iterate the path backward to calculate
    // the initial part that exists, then forward to create missing
    // directories.
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

        while (*s && isDirSep(*s))
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
        throw Error{
            std::string{"_fileno(): "} + std::strerror(errno)};

    if (_commit(fd) == -1)
        throw Error{
            std::string{"_commit(): "} + std::strerror(errno)};
}


void syncDir(const char* dirPath)
{
    // Windows doesn't support directory synchronization.
    (void)dirPath;
}


}
