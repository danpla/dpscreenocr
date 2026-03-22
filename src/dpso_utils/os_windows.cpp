#include "os.h"

#include <cstring>
#include <io.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "windows/cmdline.h"
#include "windows/com.h"
#include "windows/error.h"
#include "windows/handle.h"
#include "windows/utf.h"

#include "os_stdio.h"
#include "str.h"


namespace dpso::os {
namespace {


[[noreturn]]
void throwLastError(const char* description)
{
    const auto lastError = GetLastError();

    const auto message = str::format(
        "{}: {}", description, windows::getErrorMessage(lastError));

    switch (lastError) {
    case ERROR_FILE_NOT_FOUND:
        throw FileNotFoundError{message};
    default:
        throw Error{message};
    }
}


std::wstring toUtf16(std::string_view str, const char* varName)
{
    try {
        return windows::utf8ToUtf16(str);
    } catch (windows::CharConversionError& e) {
        throw Error{str::format(
            "Can't convert {} to UTF-16: {}", varName, e.what())};
    }
}


#define DPSO_WIN_TO_UTF16(VAR_NAME) toUtf16(VAR_NAME, #VAR_NAME)


}


std::string getErrnoMsg(int errnum)
{
    // On Windows, strerror() does not depend on the locale and is
    // documented to return a pointer to a thread-local storage.
    return std::strerror(errnum);
}


const std::string_view newline{"\r\n"};
const std::string_view dirSeparators{"\\/"};


// Returns the checked result of WideCharToMultiByte(), always > 0.
static int utf16ToAcp(
    std::wstring_view utf16Str, char* dst, int dstSize)
{
    if (utf16Str.empty())
       return 0;

    BOOL defaultCharUsed{};

    const auto size = WideCharToMultiByte(
        CP_ACP,
        WC_NO_BEST_FIT_CHARS,
        utf16Str.data(), utf16Str.size(),
        dst, dstSize,
        nullptr, &defaultCharUsed);

    if (size <= 0)
        throwLastError("WideCharToMultiByte(CP_ACP, ...)");

    if (defaultCharUsed)
        throw Error{str::format(
            "UTF-16 string contains characters that cannot be "
            "represented by the current code page (cp{})",
            GetACP())};

    return size;
}


static std::string utf16ToAcp(std::wstring_view utf16Str)
{
    std::string result(utf16ToAcp(utf16Str, nullptr, 0), 0);
    utf16ToAcp(utf16Str, result.data(), result.size());
    return result;
}


std::string convertUtf8PathToSys(const char* utf8Path)
{
    if (GetACP() == CP_UTF8)
        return utf8Path;

    return utf16ToAcp(DPSO_WIN_TO_UTF16(utf8Path));
}


FILE* fopen(const char* filePath, const char* mode)
{
    try {
        return _wfopen(
            DPSO_WIN_TO_UTF16(filePath).c_str(),
            DPSO_WIN_TO_UTF16(mode).c_str());
    } catch (Error&) {
        errno = EINVAL;
        return nullptr;
    }
}


void syncFile(FILE* fp)
{
    const auto fd = _fileno(fp);
    if (fd == -1)
        throwErrno("_fileno()", errno);

    if (_commit(fd) == -1)
        throwErrno("_commit()", errno);
}


void syncDir(const char* /*dirPath*/)
{
    // Windows doesn't support directory synchronization.
}


static void validateExePath(const char* exePath)
{
    while (str::isSpace(*exePath))
        ++exePath;

    // ShellExecute() opens the current working directory in Explorer
    // if the executable path is empty.
    if (!*exePath)
        throw Error{"Path is empty"};

    // We can't allow executing batch scripts, because it's impossible
    // to safely pass arbitrary text to them. The ^-escaping rules for
    // variables are broken by design: for example, unescaping happens
    // every time a variable is accessed, even on assignment to
    // another variable.

    auto ext = getFileExt(exePath);
    if (ext.empty())
        // If the file name does not have an extension, ShellExecute()
        // will try to execute a file with ".BAT" and ".CMD" appended.
        throw Error{"Path does not have a file extension"};

    // The string can contain trailing whitespace that will be
    // stripped by ShellExecute().
    while (!ext.empty() && str::isSpace(ext.back()))
        ext.pop_back();

    for (const auto* batchExt : {".bat", ".cmd"})
        if (str::equalIgnoreCase(batchExt, ext))
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
    const auto cmdLineUtf16 = DPSO_WIN_TO_UTF16(cmdLine);

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
