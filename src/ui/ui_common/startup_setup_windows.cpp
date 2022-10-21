
#include "startup_setup.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shobjidl.h>

#include "dpso/backend/windows/utils/com.h"
#include "dpso/backend/windows/utils/error.h"
#include "dpso/error.h"
#include "dpso/ocr.h"
#include "dpso_utils/user_dirs.h"
#include "dpso_utils/windows_utils.h"

#include "app_dirs.h"
#include "file_names.h"
#include "ocr_data_dir_name.h"


// The main goal of registering restart is to give the installer
// (e.g. Inno Setup) an ability to restart our application in case it
// was automatically closed before installing an update.
static void registerApplicationRestart()
{
    const auto* cmdLine = GetCommandLineW();

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
static bool entryExists(const wchar_t* path)
{
    return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}


static HRESULT shCreateItemFromParsingName(
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


static bool shellCopy(
    const wchar_t* srcPath, const wchar_t* dstDirPath)
{
    const dpso::windows::CoInitializer coInitializer{
        COINIT_APARTMENTTHREADED};
    if (!dpso::windows::coInitSuccess(coInitializer.getHresult())) {
        dpsoSetError(
            "COM initialization failed: %s",
            dpso::windows::getHresultMessage(
                coInitializer.getHresult()).c_str());
        return false;
    }

    dpso::windows::CoUPtr<IShellItem> srcSi;
    auto hresult = shCreateItemFromParsingName(
        srcPath, nullptr, srcSi);
    if (FAILED(hresult)) {
        dpsoSetError(
            "Can't create IShellItem for srcPath: %s",
            dpso::windows::getHresultMessage(hresult).c_str());
        return false;
    }

    dpso::windows::CoUPtr<IShellItem> dstDirSi;
    hresult = shCreateItemFromParsingName(
        dstDirPath, nullptr, dstDirSi);
    if (FAILED(hresult)) {
        dpsoSetError(
            "Can't create IShellItem for dstDirPath: %s",
            dpso::windows::getHresultMessage(hresult).c_str());
        return false;
    }

    dpso::windows::CoUPtr<IFileOperation> fileOp;
    hresult = dpso::windows::coCreateInstance(
        CLSID_FileOperation, nullptr, CLSCTX_INPROC_SERVER, fileOp);
    if (FAILED(hresult)) {
        dpsoSetError(
            "Can't create IFileOperation: %s",
            dpso::windows::getHresultMessage(hresult).c_str());
        return false;
    }

    // IFileOperation will handle the case when srcPath and dstDirPath
    // are equal.
    hresult = fileOp->CopyItem(
        srcSi.get(), dstDirSi.get(), nullptr, nullptr);
    if (FAILED(hresult)) {
        dpsoSetError(
            "IFileOperation::CopyItem(): %s",
            dpso::windows::getHresultMessage(hresult).c_str());
        return false;
    }

    hresult = fileOp->PerformOperations();
    if (FAILED(hresult)) {
        dpsoSetError(
            "IFileOperation::PerformOperations(): %s",
            dpso::windows::getHresultMessage(hresult).c_str());
        return false;
    }

    return true;
}


// Copy the given entry (file or directory) to dstDir if it exists in
// srcDir but doesn't exist in dstDir.
static bool setupEntry(
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
        } catch (std::runtime_error& e) {
        }

        dpsoSetError(
            "Can't copy \"%s\" to \"%s\": %s",
            srcPathUtf8.c_str(), dstDirUtf8.c_str(), dpsoGetError());
        return false;
    }

    return true;
}


static bool setupOcrData(
    const wchar_t* srcDataDir, const wchar_t* userDataDir)
{
    for (int i = 0; i < dpsoOcrGetNumEngines(); ++i) {
        DpsoOcrEngineInfo ocrEngineInfo;
        dpsoOcrGetEngineInfo(i, &ocrEngineInfo);

        std::wstring dirName;
        if (const auto* name = uiGetOcrDataDirName(&ocrEngineInfo))
            try {
                dirName = dpso::windows::utf8ToUtf16(name);
            } catch (std::runtime_error& e) {
            }

        if (!dirName.empty() &&
                !setupEntry(dirName.c_str(), srcDataDir, userDataDir))
            return false;
    }

    return true;
}


static int setupUserData(const wchar_t* userDataDir)
{
    std::wstring srcDataDir;
    try {
        srcDataDir = dpso::windows::utf8ToUtf16(
            uiGetAppDir(UiAppDirData));
    } catch (std::runtime_error& e) {
        dpsoSetError(
            "Can't convert UiDirData to UTF-16: %s", e.what());
        return false;
    }

    return setupOcrData(srcDataDir.c_str(), userDataDir);
}


bool uiStartupSetup(void)
{
    registerApplicationRestart();

    const auto* userDataDir = dpsoGetUserDir(
        DpsoUserDirData, uiAppFileName);
    if (!userDataDir) {
        dpsoSetError("Can't get user data dir: %s", dpsoGetError());
        return false;
    }

    std::wstring userDataDirUtf16;
    try {
        userDataDirUtf16 = dpso::windows::utf8ToUtf16(userDataDir);
    } catch (std::runtime_error& e) {
        dpsoSetError(
            "Can't convert userDataDir to UTF-16: %s", e.what());
        return false;
    }

    return setupUserData(userDataDirUtf16.c_str());
}
