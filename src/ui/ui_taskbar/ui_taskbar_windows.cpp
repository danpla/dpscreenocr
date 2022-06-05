
#include "ui_taskbar.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shobjidl.h>
#include <versionhelpers.h>

#include <utility>

#include "dpso/backend/windows/utils.h"
#include "dpso/backend/windows/utils_com.h"
#include "dpso/error.h"


struct UiTaskbar {
    dpso::windows::CoInitializer coInitializer;
    ITaskbarList3* tbl3;
    HWND hwnd;
};


struct UiTaskbar* uiTaskbarCreateWin(HWND hwnd)
{
    if (!hwnd) {
        dpsoSetError("hwnd is null");
        return nullptr;
    }

    if (!IsWindows7OrGreater()) {
        dpsoSetError(
            "ITaskbarList3 is only available on Windows 7 or newer");
        return nullptr;
    }

    dpso::windows::CoInitializer coInitializer{
        COINIT_APARTMENTTHREADED};
    if (!dpso::windows::coInitSuccess(coInitializer.getHresult())) {
        dpsoSetError(
            "COM initialization failed: %s",
            dpso::windows::getHresultMessage(
                coInitializer.getHresult()).c_str());
        return nullptr;
    }

    ITaskbarList3* tbl3;

    auto hresult = CoCreateInstance(
        CLSID_TaskbarList,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITaskbarList3,
        (void**)&tbl3);
    if (FAILED(hresult)) {
        dpsoSetError(
            "CoCreateInstance() with IID_ITaskbarList3 failed: %s",
            dpso::windows::getHresultMessage(hresult).c_str());
        return nullptr;
    }

    hresult = tbl3->HrInit();
    if (FAILED(hresult)) {
        tbl3->Release();
        dpsoSetError(
            "HrInit() failed: %s",
            dpso::windows::getHresultMessage(hresult).c_str());
        return nullptr;
    }

    return new UiTaskbar{std::move(coInitializer), tbl3, hwnd};
}


void uiTaskbarDelete(struct UiTaskbar* tb)
{
    if (!tb)
        return;

    tb->tbl3->Release();
    delete tb;
}


void uiTaskbarSetState(struct UiTaskbar* tb, UiTaskbarState newState)
{
    if (!tb)
        return;

    TBPFLAG tbpFlag{};
    switch (newState) {
        case UiTaskbarStateNormal:
            tbpFlag = TBPF_NOPROGRESS;
            break;
        case UiTaskbarStateProgress:
            tbpFlag = TBPF_NORMAL;
            break;
        case UiTaskbarStateError:
            tbpFlag = TBPF_ERROR;
            break;
    }

    // Make sure that the whole button has red background. Although
    // the documentation states that moving to TBPF_ERROR from
    // TBPF_INDETERMINATE should show "a generic percentage not
    // indicative of actual progress", it doesn't work well on
    // Windows 7, where it shows just a small red bar as if the
    // progress is set to ~1%. On Windows 10 it looks a bit better,
    // since the button gets a red underline.
    //
    // Also, TBPF_INDETERMINATE itself will not be visible if
    // animations are disabled in the system settings, so explicit
    // full progress and TBPF_ERROR is more reliable anyway.
    //
    // Another way to display error is an overlay icon, but such icons
    // are hidden if the taskbar is configured to use small buttons.
    if (tbpFlag == TBPF_ERROR)
        tb->tbl3->SetProgressValue(tb->hwnd, 100, 100);

    tb->tbl3->SetProgressState(tb->hwnd, tbpFlag);
}


void uiTaskbarSetProgress(struct UiTaskbar* tb, int progress)
{
    if (!tb)
        return;

    tb->tbl3->SetProgressValue(tb->hwnd, progress, 100);
}
