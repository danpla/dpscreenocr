// All WinRT-specific code resides in a DLL library that we load on
// demand only when the program is an MSIX package. This way, we can
// support Windows versions older than 10 and use the same build for
// both the traditional installer and MSIX (in the former case, the
// DLL can also be excluded from the distribution).

#include "msix_helper/msix_helper.h"

#include <cstddef>
#include <stdexcept>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dpso_utils/os_error.h"
#include "dpso_utils/str.h"
#include "dpso_utils/windows/error.h"
#include "dpso_utils/windows/exe_path.h"
#include "dpso_utils/windows/module.h"
#include "dpso_utils/windows/utf.h"

#include "msix_helper/dll.h"
#include "msix_helper/dll_name.h"


using namespace dpso;


namespace ui::msix {
namespace {


bool checkIsInMsix()
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


class Dll {
    #define DECL_FN(NAME) decltype(&MsixHelper_ ## NAME) NAME

    windows::ModuleUPtr lib;
    DECL_FN(getLastError);
    DECL_FN(init);
    DECL_FN(shutdown);
public:
    class Error : public std::runtime_error {
        using runtime_error::runtime_error;
    };

    // Throws Dll::Error.
    static const Dll& get()
    {
        static const Dll dll;
        return dll;
    }

    std::string_view getError() const
    {
        std::size_t len{};
        const auto* text = getLastError(&len);

        return {text, len};
    }

    DECL_FN(isActivatedByStartupTask);
    DECL_FN(startupTaskCreate);
    DECL_FN(startupTaskDelete);
    DECL_FN(startupTaskGetIsAvailable);
    DECL_FN(startupTaskGetIsEnabled);
    DECL_FN(startupTaskSetIsEnabled);

    #undef DECL_FN
private:
    static HMODULE loadLib()
    {
        // For compatibility with Windows 7-8, we manually compute the
        // absolute path to DLL instead of using
        // LOAD_LIBRARY_SEARCH_APPLICATION_DIR with LoadLibraryExW.
        // However, this doesn't make any practical difference because
        // we only load the DLL on Windows 10+ when inside MSIX.
        std::string dllPath;
        try {
            dllPath = windows::getExePath();
        } catch (os::Error& e) {
            throw Error{str::format(
                "windows::getExePath(): {}", e.what())};
        }

        if (const auto p = dllPath.rfind('\\'); p != dllPath.npos)
            dllPath.resize(p + 1);

        dllPath += helperDllName;
        dllPath += ".dll";

        auto result = LoadLibraryW(
            windows::utf8ToUtf16(dllPath).c_str());
        if (!result)
            throw Error{str::format(
                "Can't load \"{}.dll\": {}",
                helperDllName,
                windows::getErrorMessage(GetLastError()))};

        return result;
    }

    static void* loadFn(HMODULE lib, const char* name)
    {
        auto* result = (void*)GetProcAddress(lib, name);
        if (!result)
            throw Error{str::format(
                "Can't load \"{}\" from \"{}.dll\": {}",
                name,
                helperDllName,
                windows::getErrorMessage(GetLastError()))};

        return result;
    }

    #define LOAD_FN(NAME) \
        NAME{(decltype(NAME))loadFn(lib.get(), "MsixHelper_" #NAME)}

    Dll()
        : lib{loadLib()}
        , LOAD_FN(getLastError)
        , LOAD_FN(init)
        , LOAD_FN(shutdown)
        , LOAD_FN(isActivatedByStartupTask)
        , LOAD_FN(startupTaskCreate)
        , LOAD_FN(startupTaskDelete)
        , LOAD_FN(startupTaskGetIsAvailable)
        , LOAD_FN(startupTaskGetIsEnabled)
        , LOAD_FN(startupTaskSetIsEnabled)
    {
        if (!init())
            throw Error{str::format(
                "Dll::init(): {}", getError())};
    }

    #undef LOAD_FN

    ~Dll()
    {
        shutdown();
    }
};


}


bool isInMsix()
{
    static const auto result = checkIsInMsix();
    return result;
}


bool isActivatedByStartupTask()
{
    if (!isInMsix())
        return false;

    try {
        return Dll::get().isActivatedByStartupTask();
    } catch (Dll::Error&) {
        return false;
    }
}


namespace {


class StartupTaskAutostart : public ui::Autostart {
public:
    StartupTaskAutostart(const Dll& dll, const Args& args)
        : dll{dll}
    {
        std::wstring id;
        try {
            id = windows::utf8ToUtf16(args.appFileName);
        } catch (windows::CharConversionError& e) {
            throw Autostart::Error{str::format(
                "Can't convert args.appFileName to UTF-16: {}",
                e.what())};
        }

        st = dll.startupTaskCreate(id.c_str());
        if (!st)
            throw Autostart::Error{str::format(
                "startupTaskCreate: {}", dll.getError())};
    }

    ~StartupTaskAutostart()
    {
        dll.startupTaskDelete(st);
    }

    bool getIsAvailable() const override
    {
        return dll.startupTaskGetIsAvailable(st);
    }

    bool getIsEnabled() const override
    {
        return dll.startupTaskGetIsEnabled(st);
    }

    void setIsEnabled(bool newIsEnabled) override
    {
        switch (dll.startupTaskSetIsEnabled(st, newIsEnabled)) {
        case MsixHelper_StartupTaskSateChangeResultSuccess:
            return;
        case MsixHelper_StartupTaskSateChangeResultDenied:
            throw Autostart::AutostartDeniedError{
                std::string{dll.getError()}};
        case MsixHelper_StartupTaskSateChangeResultError:
            throw Autostart::Error{str::format(
                "startupTaskSetIsEnabled: {}", dll.getError())};
        }
    }
private:
    const Dll& dll;
    MsixHelper_StartupTask* st;
};


}


std::unique_ptr<Autostart> createStartupTaskAutostart(
    const Autostart::Args& args)
{
    if (!isInMsix())
        throw Autostart::Error{"Not in MSIX"};

    try {
        return std::make_unique<StartupTaskAutostart>(
            Dll::get(), args);
    } catch (Dll::Error& e) {
        throw Autostart::Error{e.what()};
    }
}


}
