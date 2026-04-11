#include "windows/package.h"

#include "windows/module.h"


namespace dpso::windows {


static bool check()
{
    auto kernel32Dll = GetModuleHandleW(L"kernel32");
    if (!kernel32Dll)
        return false;

    DPSO_WIN_DLL_FN(
        kernel32Dll,
        GetCurrentPackageFullName,
        LONG (WINAPI *)(UINT32*, PWSTR));
    if (!GetCurrentPackageFullNameFn)
        return false;

    UINT32 length{};
    LONG rc = GetCurrentPackageFullNameFn(&length, nullptr);
    return rc == ERROR_INSUFFICIENT_BUFFER;
}


bool isInPackage()
{
    static const auto result = check();
    return result;
}


}
