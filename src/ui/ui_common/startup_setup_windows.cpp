
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
#include "file_names.h"
#include "ocr_data_dir_name.h"


// Returns true if a file system entry (file, dir, etc.) exists.
static bool entryExists(const wchar_t* path)
{
    return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}


// Returns an empty string on error.
static std::wstring getExeDir()
{
    std::wstring result(32, 0);

    while (true) {
        const auto size = GetModuleFileNameW(
            nullptr, &result[0], result.size());

        if (size == 0) {
            dpsoSetError(
                "GetModuleFileNameW() failed: %s",
                dpso::windows::getErrorMessage(
                    GetLastError()).c_str());
            return {};
        }

        if (size < result.size())
            break;

        result.resize(result.size() * 2);
    }

    const auto slashPos = result.rfind(L'\\');
    if (slashPos != result.npos)
        result.resize(slashPos);

    return result;
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
            "IFileOperation::CopyItem() failed: %s",
            dpso::windows::getHresultMessage(hresult).c_str());
        return false;
    }

    hresult = fileOp->PerformOperations();
    if (FAILED(hresult)) {
        dpsoSetError(
            "IFileOperation::PerformOperations() failed: %s",
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

    const auto srcPath = (
        std::wstring{srcDir} + L'\\' + entryName);
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
    const wchar_t* exeDir, const wchar_t* userDataDir)
{
    for (int i = 0; i < dpsoOcrGetNumEngines(); ++i) {
        DpsoOcrEngineInfo ocrEngineInfo;
        dpsoOcrGetEngineInfo(i, &ocrEngineInfo);

        std::wstring dirName;
        if (const auto* name = getOcrDataDirName(&ocrEngineInfo))
            try {
                dirName = dpso::windows::utf8ToUtf16(name);
            } catch (std::runtime_error& e) {
            }

        if (!dirName.empty() &&
                !setupEntry(dirName.c_str(), exeDir, userDataDir))
            return false;
    }

    return true;
}


static int setupUserData(const wchar_t* userDataDir)
{
    const auto exeDir = getExeDir();
    if (exeDir.empty()) {
        dpsoSetError("getExeDir() filed: %s", dpsoGetError());
        return false;
    }

    return setupOcrData(exeDir.c_str(), userDataDir);
}


int startupSetup(int portableMode)
{
    if (portableMode)
        return true;

    const auto* userDataDir = dpsoGetUserDir(
        DpsoUserDirData, appFileName);
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
