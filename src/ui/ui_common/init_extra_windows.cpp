#include "init_extra.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>

#include "dpso_ext/user_dirs.h"
#include "dpso_utils/error_get.h"
#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"
#include "dpso_utils/windows/com.h"
#include "dpso_utils/windows/error.h"
#include "dpso_utils/windows/utf.h"

#include "app_dirs.h"
#include "file_names.h"
#include "ocr_default_data_dir.h"


using namespace dpso;


namespace ui {
namespace {


// The main goal of registering restart is to give the installer
// (e.g. Inno Setup) an ability to restart our application in case it
// was automatically closed before installing an update.
void registerApplicationRestart()
{
    const auto* cmdLine = GetCommandLineW();
    // The command line is actually never empty, but check anyway for
    // the code below.
    if (!*cmdLine)
        return;

    // RegisterApplicationRestart() doesn't need the path to the
    // executable, so skip it. It may be in double quotes if it
    // contains spaces.
    const auto endChar = *cmdLine++ == L'\"' ? L'\"' : L' ';
    while (*cmdLine)
        if (*cmdLine++ == endChar)
            break;

    while (*cmdLine == L' ')
        ++cmdLine;

    RegisterApplicationRestart(
        cmdLine, RESTART_NO_CRASH | RESTART_NO_HANG);
}


HRESULT shCreateItemFromParsingName(
    const wchar_t* name,
    IBindCtx *pbc,
    windows::CoUPtr<IShellItem>& ptr)
{
    IShellItem* rawPtr;
    const auto hresult = SHCreateItemFromParsingName(
        name, pbc, IID_PPV_ARGS(&rawPtr));

    ptr.reset(rawPtr);
    return hresult;
}


bool shellCopy(const wchar_t* srcPath, const wchar_t* dstDirPath)
{
    const windows::CoInitializer coInitializer{
        COINIT_APARTMENTTHREADED};
    if (!windows::coInitSuccess(coInitializer.getHresult())) {
        setError(
            "COM initialization failed: {}",
            windows::getHresultMessage(coInitializer.getHresult()));
        return false;
    }

    windows::CoUPtr<IShellItem> srcSi;
    auto hresult = shCreateItemFromParsingName(
        srcPath, nullptr, srcSi);
    if (FAILED(hresult)) {
        setError(
            "Can't create IShellItem for srcPath: {}",
            windows::getHresultMessage(hresult));
        return false;
    }

    windows::CoUPtr<IShellItem> dstDirSi;
    hresult = shCreateItemFromParsingName(
        dstDirPath, nullptr, dstDirSi);
    if (FAILED(hresult)) {
        setError(
            "Can't create IShellItem for dstDirPath: {}",
            windows::getHresultMessage(hresult));
        return false;
    }

    windows::CoUPtr<IFileOperation> fileOp;
    hresult = windows::coCreateInstance(
        CLSID_FileOperation, nullptr, CLSCTX_INPROC_SERVER, fileOp);
    if (FAILED(hresult)) {
        setError(
            "Can't create IFileOperation: {}",
            windows::getHresultMessage(hresult));
        return false;
    }

    // IFileOperation will handle the case when srcPath and dstDirPath
    // are equal.
    hresult = fileOp->CopyItem(
        srcSi.get(), dstDirSi.get(), nullptr, nullptr);
    if (FAILED(hresult)) {
        setError(
            "IFileOperation::CopyItem(): {}",
            windows::getHresultMessage(hresult));
        return false;
    }

    // The default operation flags are FOF_ALLOWUNDO and
    // FOF_NOCONFIRMMKDIR. We explicitly set flags to hide the
    // progress dialog (since it allows canceling the operation) and
    // generally disable all other interactivity.
    hresult = fileOp->SetOperationFlags(
        FOF_NOCONFIRMMKDIR
        | FOF_NOERRORUI
        | FOFX_EARLYFAILURE
        | FOF_SILENT);
    if (FAILED(hresult)) {
        setError(
            "IFileOperation::SetOperationFlags(): {}",
            windows::getHresultMessage(hresult));
        return false;
    }

    hresult = fileOp->PerformOperations();
    if (FAILED(hresult)) {
        setError(
            "IFileOperation::PerformOperations(): {}",
            windows::getHresultMessage(hresult));
        return false;
    }

    return true;
}


bool shellCopy(const char* srcPath, const char* dstDirPath)
{
    std::wstring srcPathUtf16;
    try {
        srcPathUtf16 = windows::utf8ToUtf16(srcPath);
    } catch (windows::CharConversionError& e) {
        setError(
            "Can't convert srcPath to UTF-16: {}", e.what());
        return false;
    }

    std::wstring dstDirPathUtf16;
    try {
        dstDirPathUtf16 = windows::utf8ToUtf16(dstDirPath);
    } catch (windows::CharConversionError& e) {
        setError(
            "Can't convert dstDirPath to UTF-16: {}", e.what());
        return false;
    }

    return shellCopy(srcPathUtf16.c_str(), dstDirPathUtf16.c_str());
}


// Return true if a file system entry (file, dir, etc.) exists.
bool entryExists(const char* path)
{
    std::wstring pathUtf16;
    try {
        pathUtf16 = windows::utf8ToUtf16(path);
    } catch (windows::CharConversionError& e) {
        return false;
    }

    return GetFileAttributesW(pathUtf16.c_str())
        != INVALID_FILE_ATTRIBUTES;
}


// If a file or directory srcPath exists, copy it to dstDirPath if it
// doesn't already exist there.
bool setupEntry(const char* srcPath, const char* dstDirPath)
{
    const auto entryName = os::getBaseName(srcPath);
    if (entryName.empty())
        return true;

    if (entryExists(
            (std::string{dstDirPath} + '\\' + entryName).c_str()))
        return true;

    if (!entryExists(srcPath))
        return true;

    try {
        os::makeDirs(dstDirPath);
    } catch (os::Error& e) {
        setError("os::makeDirs(\"{}\"): {}", dstDirPath, e.what());
        return false;
    }

    if (!shellCopy(srcPath, dstDirPath)) {
        setError(
            "Can't copy \"{}\" to \"{}\": {}",
            srcPath, dstDirPath, dpsoGetError());
        return false;
    }

    return true;
}


bool setupOcrData()
{
    const std::string appDataDir{uiGetAppDir(UiAppDirData)};

    for (int i = 0; i < dpsoOcrGetNumEngines(); ++i) {
        DpsoOcrEngineInfo ocrEngineInfo;
        dpsoOcrGetEngineInfo(i, &ocrEngineInfo);

        const auto dataDirPath = getDefaultOcrDataDir(ocrEngineInfo);
        if (!dataDirPath)
            return false;

        const auto dataDirName = os::getBaseName(
            dataDirPath->c_str());
        if (dataDirName.empty())
            continue;

        const auto dataDirParentPath = os::getDirName(
            dataDirPath->c_str());
        if (dataDirParentPath.empty())
            continue;

        if (!setupEntry(
                (appDataDir + '\\' + dataDirName).c_str(),
                dataDirParentPath.c_str()))
            return false;
    }

    return true;
}


}


bool initStart(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    return true;
}


bool initEnd(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    registerApplicationRestart();

    if (!setupOcrData()) {
        setError("setupOcrData(): {}", dpsoGetError());
        return false;
    }

    return true;
}


}
