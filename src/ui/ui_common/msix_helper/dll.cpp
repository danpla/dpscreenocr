#include "msix_helper/dll.h"

#include <future>
#include <stdexcept>
#include <string>
#include <utility>

#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.Foundation.h>


using namespace winrt::Windows::ApplicationModel;


#define N_(S) S


namespace {


// The error message for the C API.
std::string lastError;


void setLastError(
    std::string_view info, const winrt::hresult_error& e)
{
    lastError = info;
    lastError += ": ";
    lastError += winrt::to_string(e.message());
}


class Error : public std::runtime_error {
public:
    using runtime_error::runtime_error;

    Error(std::string_view info, const winrt::hresult_error& e)
        : runtime_error{
            std::string{info} + ": " + winrt::to_string(e.message())}
    {
    }
};


// Our main thread uses the single-threaded apartment, but async
// methods of WinRT (e.g. those that return IAsyncOperation for which
// we need to call ::get()) must only be run in threads that use the
// multi-threaded apartment, because running them in STA can result in
// a deadlock.
template<typename Fn>
auto runInMtaThread(Fn&& fn)
{
    return std::async(
        std::launch::async,
        [fn = std::forward<Fn>(fn)]()
        {
            try {
                winrt::init_apartment(
                    winrt::apartment_type::multi_threaded);
            } catch (winrt::hresult_error& e) {
                throw Error{
                    "runInMtaThread(): winrt::init_apartment()", e};
            }

            // std::async() can use a thread pool, so it's important
            // to de-initialize the apartment.
            const struct UninitApartment {
                ~UninitApartment() {
                    winrt::uninit_apartment();
                }
            } uninitApartment;

            return fn();
        }).get();
}


}


const char* MsixHelper_getLastError(std::size_t* len)
{
    if (len)
        *len = lastError.size();

    return lastError.c_str();
}


bool MsixHelper_init()
{
    try {
        // Note that init_apartment() is nothing more than a wrapper
        // for CoInitializeEx(), so apartment_type must match COINIT_*
        // vale we use for CoInitializeEx() in our code.
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        return true;
    } catch (winrt::hresult_error& e) {
        setLastError("winrt::init_apartment()", e);
        return false;
    }
}


void MsixHelper_shutdown()
{
    winrt::uninit_apartment();
}


bool MsixHelper_isActivatedByStartupTask()
{
    try {
        return
            AppInstance::GetActivatedEventArgs().Kind()
            == Activation::ActivationKind::StartupTask;
    } catch (winrt::hresult_error& e) {
        return false;
    }
}


namespace {


class StartupTaskDeniedError : public Error {
    using Error::Error;
};


StartupTaskState getStartupTaskState(const std::wstring& id)
{
    return runInMtaThread(
        [&]()
        {
            try {
                return StartupTask::GetAsync(id).get().State();
            } catch (winrt::hresult_error& e) {
                throw Error{"StartupTask::GetAsync().State()", e};
            }
        });
}


bool isEnabled(StartupTaskState state)
{
    return state == StartupTaskState::Enabled
        || state == StartupTaskState::EnabledByPolicy;
}


void setStatupTaskIsEnabled(const std::wstring& id, bool newIsEnabled)
{
    StartupTask startupTask{nullptr};
    try {
        startupTask = StartupTask::GetAsync(id).get();
    } catch (winrt::hresult_error& e) {
        throw Error{"StartupTask::GetAsync()", e};
    }

    StartupTaskState state;
    try {
        state = startupTask.State();
    } catch (winrt::hresult_error& e) {
        throw Error{"StartupTask::State()", e};
    }

    if (isEnabled(state) == newIsEnabled)
        return;

    if (newIsEnabled) {
        if (state == StartupTaskState::DisabledByUser)
            // Translators: This is a Windows-specific message. Make
            // sure that "Settings", "Apps", and "Startup" actually
            // match the text you see in the Windows interface in the
            // target language. If you don't have access to Windows,
            // look for screenshots on the Internet (try searching for
            // "windows settings startup").
            //
            // https://en.wikipedia.org/wiki/Settings_(Windows)
            // https://support.microsoft.com/en-us/windows/configure-startup-applications-in-windows-115a420a-0bff-4a6f-90e0-1934c844e473
            throw StartupTaskDeniedError{N_(
                "Startup is disabled by the user. You can enable it "
                "in Windows Settings (Apps > Startup).")};

        if (state == StartupTaskState::DisabledByPolicy)
            throw Error{
                "Startup is disabled by the administrator or group "
                "policy, or is not supported on this platform"};

        try {
            state = startupTask.RequestEnableAsync().get();
        } catch (winrt::hresult_error& e) {
            throw Error{"StartupTask::RequestEnableAsync()", e};
        }

        if (!isEnabled(state))
            throw Error{
                "Can't enable StartupTask. Current state is: "
                + std::to_string(
                    static_cast<
                        std::underlying_type_t<StartupTaskState>>(
                            state))};

        return;
    }

    if (state == StartupTaskState::EnabledByPolicy)
        throw Error{
            "Startup is enabled by the administrator or group "
            "policy"};

    try {
        startupTask.Disable();
    } catch (winrt::hresult_error& e) {
        throw Error{"StartupTask::Disable()", e};
    }
}


}


struct MsixHelper_StartupTask {
    std::wstring id;
};


MsixHelper_StartupTask*
    MsixHelper_startupTaskCreate(const wchar_t* id)
{
    return new MsixHelper_StartupTask{id};
}


void MsixHelper_startupTaskDelete(MsixHelper_StartupTask* st)
{
    delete st;
}


bool
MsixHelper_startupTaskGetIsAvailable(const MsixHelper_StartupTask* st)
{
    if (!st)
        return false;

    try {
        switch (getStartupTaskState(st->id)) {
        case StartupTaskState::Disabled:
        case StartupTaskState::DisabledByUser:
        case StartupTaskState::Enabled:
            return true;
        case StartupTaskState::DisabledByPolicy:
        case StartupTaskState::EnabledByPolicy:
            break;
        }
    } catch (Error&) {
    }

    return false;
}


bool
MsixHelper_startupTaskGetIsEnabled(const MsixHelper_StartupTask* st)
{
    if (!st)
        return false;

    try {
        return isEnabled(getStartupTaskState(st->id));
    } catch (Error&) {
        return false;
    }
}


MsixHelper_StartupTaskSateChangeResult
MsixHelper_startupTaskSetIsEnabled(
    MsixHelper_StartupTask* st, bool newIsEnabled)
{
    if (!st) {
        lastError = "st is null";
        return MsixHelper_StartupTaskSateChangeResultError;
    }

    try {
        runInMtaThread(
            [&]()
            {
                setStatupTaskIsEnabled(st->id, newIsEnabled);
            });
        return MsixHelper_StartupTaskSateChangeResultSuccess;
    } catch (StartupTaskDeniedError& e) {
        lastError = e.what();
        return MsixHelper_StartupTaskSateChangeResultDenied;
    } catch (Error& e) {
        lastError = e.what();
        return MsixHelper_StartupTaskSateChangeResultError;
    }
}
