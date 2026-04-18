#include "autostart/autostart_windows_registry.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dpso_utils/str.h"
#include "dpso_utils/windows/cmdline.h"
#include "dpso_utils/windows/error.h"
#include "dpso_utils/windows/handle.h"
#include "dpso_utils/windows/utf.h"


using namespace dpso;


namespace ui {
namespace {


const auto regKey = HKEY_CURRENT_USER;
const auto* regSubKey =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";


// A RegGetValueW() helper for REG_SZ type returning std::wstring.
LSTATUS regGetStr(
    HKEY key,
    const wchar_t* subKey,
    const wchar_t* value,
    std::wstring& result)
{
    const auto flags = RRF_RT_REG_SZ;

    DWORD dataSize;
    auto status = RegGetValueW(
        key, subKey, value, flags, nullptr, nullptr, &dataSize);
    if (status != ERROR_SUCCESS)
        return status;

    // dataSize is in bytes and includes the null terminator.
    if (dataSize < sizeof(wchar_t))
        return ERROR_BAD_ARGUMENTS;

    result.resize(dataSize / sizeof(wchar_t) - 1);
    return RegGetValueW(
        key, subKey, value, flags, nullptr, result.data(), &dataSize);
}


std::wstring getExePath(const std::wstring& cmdLine)
{
    if (cmdLine.empty())
        return {};

    if (cmdLine.front() == L'"') {
        const auto pos = cmdLine.find(L'"', 1);
        if (pos != cmdLine.npos)
            return cmdLine.substr(1, pos - 1);

        return {};
    }

    return cmdLine.substr(0, cmdLine.find(L' '));
}


bool getFileInfo(
    const wchar_t* path, BY_HANDLE_FILE_INFORMATION& fileInfo)
{
    windows::Handle<windows::InvalidHandleType::value> file{
        CreateFileW(
            path,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr)};

    return file && GetFileInformationByHandle(file, &fileInfo);
}


bool isSameFsEntry(const wchar_t* aPath, const wchar_t* bPath)
{
    BY_HANDLE_FILE_INFORMATION a, b;

    return
        getFileInfo(aPath, a)
        && getFileInfo(bPath, b)
        && a.dwVolumeSerialNumber == b.dwVolumeSerialNumber
        && a.nFileIndexHigh == b.nFileIndexHigh
        && a.nFileIndexLow == b.nFileIndexLow;
}


class RegistryAutostart : public Autostart {
public:
    explicit RegistryAutostart(const Args& args);

    bool getIsAvailable() const override
    {
        return true;
    }

    bool getIsEnabled() const override
    {
        return isEnabled;
    }

    void setIsEnabled(bool newIsEnabled) override;
private:
    std::wstring appName;
    std::wstring cmdLine;
    bool isEnabled{};
};



RegistryAutostart::RegistryAutostart(const Args& args)
{
    if (args.numArgs == 0)
        throw Error{"args.numArgs is 0"};

    try {
        appName = windows::utf8ToUtf16(args.appName);;
    } catch (windows::CharConversionError& e) {
        throw Error{str::format(
            "Can't convert args.appName to UTF-16: {}", e.what())};
    }

    std::wstring existingCmdLine;
    const auto status = regGetStr(
        regKey, regSubKey, appName.c_str(), existingCmdLine);
    if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND)
        throw Error{str::format(
            "regGetStr(): {}", windows::getErrorMessage(status))};

    if (status != ERROR_FILE_NOT_FOUND) {
        std::wstring exePath;
        try {
            exePath = windows::utf8ToUtf16(args.args[0]);
        } catch (windows::CharConversionError& e) {
            throw Error{str::format(
                "Can't convert args.args[0] to UTF-16: {}",
                e.what())};
        }

        const auto existingExePath = getExePath(existingCmdLine);

        // Do the string comparison first as an optimization for the
        // common case when both paths are the same.
        isEnabled =
            exePath == existingExePath
            || isSameFsEntry(
                exePath.c_str(), existingExePath.c_str());
    }

    try {
        cmdLine = windows::utf8ToUtf16(
            windows::createCmdLine(
                args.args[0], args.args + 1, args.numArgs - 1));
    } catch (windows::CharConversionError& e) {
        throw Error{str::format(
            "Can't convert command line (args.args) to UTF-16: {}",
            e.what())};
    }
}


void RegistryAutostart::setIsEnabled(bool newIsEnabled)
{
    if (newIsEnabled == isEnabled)
        return;

    if (!newIsEnabled) {
        const auto status = RegDeleteKeyValueW(
            regKey, regSubKey, appName.c_str());
        if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND)
            throw Error{str::format(
                "RegDeleteKeyValueW(): {}",
                windows::getErrorMessage(status))};

        isEnabled = false;
        return;
    }

    const auto status = RegSetKeyValueW(
        regKey,
        regSubKey,
        appName.c_str(),
        REG_SZ,
        cmdLine.c_str(),
        (cmdLine.size() + 1) * sizeof(wchar_t));
    if (status != ERROR_SUCCESS)
        throw Error{str::format(
            "RegSetKeyValueW(): {}",
            windows::getErrorMessage(status))};

    isEnabled = true;
}


}


std::unique_ptr<Autostart> createRegistryAutostart(
    const Autostart::Args& args)
{
    return std::make_unique<RegistryAutostart>(args);
}


}
