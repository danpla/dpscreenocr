#include "update_checker_platform.h"

#include <charconv>

#include <windows.h>

#include "dpso_utils/str.h"
#include "dpso_utils/windows/error.h"
#include "dpso_utils/windows/module.h"

#include "update_checker.h"


using namespace dpso;


const char* uiUpdateCheckerGetPlatformId(void)
{
    return "windows";
}


namespace ui::updateChecker {
namespace {


RTL_OSVERSIONINFOW getWindowsVersion()
{
    auto ntDll = GetModuleHandleW(L"ntdll");
    if (!ntDll)
        throw RequirementsError{str::format(
            "GetModuleHandleW(\"ntdll\"): {}",
            windows::getErrorMessage(GetLastError()))};

    DPSO_WIN_DLL_FN(
        ntDll,
        RtlGetVersion,
        NTSTATUS (NTAPI *)(PRTL_OSVERSIONINFOW));
    if (!RtlGetVersionFn)
        throw RequirementsError{str::format(
            "Can't load RtlGetVersion form ntdll: {}",
            windows::getErrorMessage(GetLastError()))};

    RTL_OSVERSIONINFOW result{};
    result.dwOSVersionInfoSize = sizeof(result);

    // RtlGetVersion is documented to always return STATUS_SUCCESS.
    RtlGetVersionFn(&result);
    return result;
}


RTL_OSVERSIONINFOW getVersionFromStr(const std::string& str)
{
    RTL_OSVERSIONINFOW result{};

    const auto* s = str.c_str();
    const auto* sEnd = s + str.size();

    const auto parse = [&](ULONG& result)
    {
        const auto [ptr, ec] = std::from_chars(s, sEnd, result);
        if (ec != std::errc{})
            return false;

        s = ptr;
        return true;
    };

    const auto consume = [&](char c)
    {
        if (s < sEnd && *s == c) {
            ++s;
            return true;
        }

        return false;
    };

    if (parse(result.dwMajorVersion)
            && consume('.')
            && parse(result.dwMinorVersion)
            && consume('.')
            && parse(result.dwBuildNumber)
            && s == sEnd)
        return result;

    throw RequirementsError{
        "String doesn't match the \"major.minor.build\" format"};
}


bool isLess(const RTL_OSVERSIONINFOW& a, const RTL_OSVERSIONINFOW& b)
{
    #define CMP(N) if (a.N != b.N) return a.N < b.N

    CMP(dwMajorVersion);
    CMP(dwMinorVersion);
    CMP(dwBuildNumber);

    #undef CMP

    return false;
}


std::string getWindowsName(const RTL_OSVERSIONINFOW& v)
{
    std::string name;

    if (v.dwMajorVersion == 6 && v.dwMinorVersion == 1)
        name = "7";
    else if (v.dwMajorVersion == 6 && v.dwMinorVersion == 2)
        name = "8";
    else if (v.dwMajorVersion == 6 && v.dwMinorVersion == 3)
        name = "8.1";
    else if (v.dwMajorVersion == 10 && v.dwMinorVersion == 0) {
        if (v.dwBuildNumber < 22000)
            name = "10";
        else if (v.dwBuildNumber <= 26200)
            name = "11";
    }

    const auto vStr = str::format(
        "{}.{}.{}",
        v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber);

    if (name.empty())
        return "Windows version " + vStr;

    return str::format("Windows {} ({})", name, vStr);
}


}


std::vector<UnmetRequirement> processRequirements(
    const dpso::json::Object& requirements)
{
    RTL_OSVERSIONINFOW actualVersion;
    try {
        actualVersion = getWindowsVersion();
    } catch (RequirementsError& e) {
        throw RequirementsError{str::format(
            "Can't get Windows version: {}", e.what())};
    }

    RTL_OSVERSIONINFOW requiredVersion;
    try {
        requiredVersion = getVersionFromStr(
            requirements.getStr("windows-version"));
    } catch (RequirementsError& e) {
        throw RequirementsError{str::format(
            "Invalid \"windows-version\": {}", e.what())};
    }

    if (isLess(actualVersion, requiredVersion))
        return {
            {
                getWindowsName(requiredVersion),
                getWindowsName(actualVersion)}};

    return {};
}


}
