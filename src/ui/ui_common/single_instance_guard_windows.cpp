
#include "single_instance_guard.h"

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dpso_utils/error_set.h"
#include "dpso_utils/windows/error.h"
#include "dpso_utils/windows/handle.h"
#include "dpso_utils/windows/utf.h"


using namespace dpso;


struct UiSingleInstanceGuard {
    windows::Handle<windows::InvalidHandleType::null> mutex;
};


UiSingleInstanceGuard* uiSingleInstanceGuardCreate(const char* id)
{
    std::wstring idUtf16;
    try {
        idUtf16 = windows::utf8ToUtf16(id);
    } catch (windows::CharConversionError& e) {
        setError("Can't convert id to UTF-16: {}", e.what());
        return {};
    }

    // Kernel objects (a mutex in our case) are in the local namespace
    // by default, but it's also possible to explicitly create them in
    // a global or local namespace by prefixing their names with
    // "Global\" or "Local\", respectively.
    //
    // We explicitly prepend "Local\" for the edge case when the id
    // starts with "Global\". Also, the rest of the name cannot have
    // backslashes, so we replace them with underscores.

    for (auto& c : idUtf16)
        if (c == L'\\')
            c = L'_';

    const auto mutexName = L"Local\\" + idUtf16 + L"_instance_mutex";

    windows::Handle<windows::InvalidHandleType::null> mutex{
        CreateMutexW(nullptr, false, mutexName.c_str())};
    if (!mutex) {
        setError(
            "CreateMutexW(..., \"{}\"): {}",
            windows::utf16ToUtf8(mutexName.c_str()),
            windows::getErrorMessage(GetLastError()));
        return {};
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
        mutex = {};

    return new UiSingleInstanceGuard{std::move(mutex)};
}


void uiSingleInstanceGuardDelete(UiSingleInstanceGuard* guard)
{
    delete guard;
}


bool uiSingleInstanceGuardIsPrimary(
    const UiSingleInstanceGuard* guard)
{
    return guard && guard->mutex;
}

