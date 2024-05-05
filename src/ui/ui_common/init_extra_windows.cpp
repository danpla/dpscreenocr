
#include "init_extra.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>

#include "dpso_ext/user_dirs.h"
#include "dpso/ocr.h"
#include "dpso_utils/error_get.h"
#include "dpso_utils/error_set.h"
#include "dpso_utils/windows/com.h"
#include "dpso_utils/windows/error.h"
#include "dpso_utils/windows/utf.h"

#include "app_dirs.h"
#include "file_names.h"
#include "ocr_data_utils.h"


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


// Returns true if a file system entry (file, dir, etc.) exists.
bool entryExists(const wchar_t* path)
{
    return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}


HRESULT shCreateItemFromParsingName(
    const wchar_t* name,
    IBindCtx *pbc,
    dpso::windows::CoUPtr<IShellItem>& ptr)
{
    IShellItem* rawPtr;
    const auto hresult = SHCreateItemFromParsingName(
        name, pbc, IID_PPV_ARGS(&rawPtr));

    ptr = dpso::windows::CoUPtr<IShellItem>{rawPtr};
    return hresult;
}


bool shellCopy(const wchar_t* srcPath, const wchar_t* dstDirPath)
{
    const dpso::windows::CoInitializer coInitializer{
        COINIT_APARTMENTTHREADED};
    if (!dpso::windows::coInitSuccess(coInitializer.getHresult())) {
        dpso::setError(
            "COM initialization failed: {}",
            dpso::windows::getHresultMessage(
                coInitializer.getHresult()));
        return false;
    }

    dpso::windows::CoUPtr<IShellItem> srcSi;
    auto hresult = shCreateItemFromParsingName(
        srcPath, nullptr, srcSi);
    if (FAILED(hresult)) {
        dpso::setError(
            "Can't create IShellItem for srcPath: {}",
            dpso::windows::getHresultMessage(hresult));
        return false;
    }

    dpso::windows::CoUPtr<IShellItem> dstDirSi;
    hresult = shCreateItemFromParsingName(
        dstDirPath, nullptr, dstDirSi);
    if (FAILED(hresult)) {
        dpso::setError(
            "Can't create IShellItem for dstDirPath: {}",
            dpso::windows::getHresultMessage(hresult));
        return false;
    }

    dpso::windows::CoUPtr<IFileOperation> fileOp;
    hresult = dpso::windows::coCreateInstance(
        CLSID_FileOperation, nullptr, CLSCTX_INPROC_SERVER, fileOp);
    if (FAILED(hresult)) {
        dpso::setError(
            "Can't create IFileOperation: {}",
            dpso::windows::getHresultMessage(hresult));
        return false;
    }

    // IFileOperation will handle the case when srcPath and dstDirPath
    // are equal.
    hresult = fileOp->CopyItem(
        srcSi.get(), dstDirSi.get(), nullptr, nullptr);
    if (FAILED(hresult)) {
        dpso::setError(
            "IFileOperation::CopyItem(): {}",
            dpso::windows::getHresultMessage(hresult));
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
        dpso::setError(
            "IFileOperation::SetOperationFlags(): {}",
            dpso::windows::getHresultMessage(hresult));
        return false;
    }

    hresult = fileOp->PerformOperations();
    if (FAILED(hresult)) {
        dpso::setError(
            "IFileOperation::PerformOperations(): {}",
            dpso::windows::getHresultMessage(hresult));
        return false;
    }

    return true;
}


// Copy the given entry (file or directory) to dstDir if it exists in
// srcDir but doesn't exist in dstDir.
bool setupEntry(
    const wchar_t* entryName,
    const wchar_t* srcDir,
    const wchar_t* dstDir)
{
    if (entryExists(
            (std::wstring{dstDir} + L'\\' + entryName).c_str()))
        return true;

    const auto srcPath = std::wstring{srcDir} + L'\\' + entryName;
    if (!entryExists(srcPath.c_str()))
        return true;

    if (!shellCopy(srcPath.c_str(), dstDir)) {
        std::string srcPathUtf8;
        std::string dstDirUtf8;
        try {
            srcPathUtf8 = dpso::windows::utf16ToUtf8(srcPath.c_str());
            dstDirUtf8 = dpso::windows::utf16ToUtf8(dstDir);
        } catch (dpso::windows::CharConversionError& e) {
        }

        dpso::setError(
            "Can't copy \"{}\" to \"{}\": {}",
            srcPathUtf8, dstDirUtf8, dpsoGetError());
        return false;
    }

    return true;
}


bool setupOcrData(
    const wchar_t* srcDataDir, const wchar_t* userDataDir)
{
    for (int i = 0; i < dpsoOcrGetNumEngines(); ++i) {
        DpsoOcrEngineInfo ocrEngineInfo;
        dpsoOcrGetEngineInfo(i, &ocrEngineInfo);

        std::wstring dirName;
        try {
            dirName = dpso::windows::utf8ToUtf16(
                uiGetOcrDataDirName(&ocrEngineInfo));
        } catch (dpso::windows::CharConversionError& e) {
        }

        if (!dirName.empty() &&
                !setupEntry(dirName.c_str(), srcDataDir, userDataDir))
            return false;
    }

    return true;
}


int setupUserData(const wchar_t* userDataDir)
{
    std::wstring srcDataDir;
    try {
        srcDataDir = dpso::windows::utf8ToUtf16(
            uiGetAppDir(UiAppDirData));
    } catch (dpso::windows::CharConversionError& e) {
        dpso::setError(
            "Can't convert UiAppDirData to UTF-16: {}", e.what());
        return false;
    }

    return setupOcrData(srcDataDir.c_str(), userDataDir);
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

    const auto* userDataDir = dpsoGetUserDir(
        DpsoUserDirData, uiAppFileName);
    if (!userDataDir) {
        dpso::setError("Can't get user data dir: {}", dpsoGetError());
        return false;
    }

    std::wstring userDataDirUtf16;
    try {
        userDataDirUtf16 = dpso::windows::utf8ToUtf16(userDataDir);
    } catch (dpso::windows::CharConversionError& e) {
        dpso::setError(
            "Can't convert userDataDir to UTF-16: {}", e.what());
        return false;
    }

    if (!setupUserData(userDataDirUtf16.c_str())) {
        dpso::setError("setupUserData: {}", dpsoGetError());
        return false;
    }

    return true;
}


}
