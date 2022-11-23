
#include "single_instance_guard.h"

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dpso/backend/windows/utils/error.h"
#include "dpso/error.h"
#include "dpso_utils/windows_utils.h"


struct UiSingleInstanceGuard {
    HANDLE mutex;
};


UiSingleInstanceGuard* uiSingleInstanceGuardCreate(const char* id)
{
    std::wstring idUtf16;
    try {
        idUtf16 = dpso::windows::utf8ToUtf16(id);
    } catch (std::runtime_error& e) {
        dpsoSetError("Can't convert id to UTF-16: %s", e.what());
        return {};
    }

    const auto mutexName = idUtf16 + L"_instance_mutex";

    const auto mutex = CreateMutexW(
        nullptr, false, mutexName.c_str());
    if (!mutex) {
        dpsoSetError(
            "CreateMutexW(..., \"%s\"): %s",
            dpso::windows::utf16ToUtf8(mutexName.c_str()).c_str(),
            dpso::windows::getErrorMessage(GetLastError()).c_str());
        return {};
    }

    return new UiSingleInstanceGuard{
        GetLastError() == ERROR_ALREADY_EXISTS ? nullptr : mutex};
}


void uiSingleInstanceGuardDelete(UiSingleInstanceGuard* guard)
{
    if (!guard)
        return;

    if (guard->mutex)
        CloseHandle(guard->mutex);

    delete guard;
}


bool uiSingleInstanceGuardIsPrimary(
    const UiSingleInstanceGuard* guard)
{
    return guard && guard->mutex;
}

