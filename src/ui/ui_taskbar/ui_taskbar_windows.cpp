
#include "ui_taskbar.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shobjidl.h>
#include <versionhelpers.h>

#include <utility>

#include "dpso/backend/windows/utils/com.h"
#include "dpso/backend/windows/utils/error.h"
#include "dpso/error.h"


struct UiTaskbar {
    dpso::windows::CoInitializer coInitializer;
    dpso::windows::CoUPtr<ITaskbarList3> tbl;
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

    dpso::windows::CoUPtr<ITaskbarList3> tbl;
    auto hresult = dpso::windows::coCreateInstance(
        CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, tbl);
    if (FAILED(hresult)) {
        dpsoSetError(
            "CoCreateInstance() for ITaskbarList3 failed: %s",
            dpso::windows::getHresultMessage(hresult).c_str());
        return nullptr;
    }

    hresult = tbl->HrInit();
    if (FAILED(hresult)) {
        dpsoSetError(
            "HrInit() failed: %s",
            dpso::windows::getHresultMessage(hresult).c_str());
        return nullptr;
    }

    return new UiTaskbar{
        std::move(coInitializer), std::move(tbl), hwnd};
}


void uiTaskbarDelete(struct UiTaskbar* tb)
{
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
            tb->tbl->SetProgressValue(tb->hwnd, 0, 100);
            break;
        case UiTaskbarStateError:
            tbpFlag = TBPF_ERROR;
            // Make sure that the whole button has red background.
            //
            // Another way to display error is an overlay icon, but
            // such icons are hidden if the taskbar is configured to
            // use small buttons.
            tb->tbl->SetProgressValue(tb->hwnd, 100, 100);
            break;
    }

    tb->tbl->SetProgressState(tb->hwnd, tbpFlag);
}


void uiTaskbarSetProgress(struct UiTaskbar* tb, int newProgress)
{
    if (!tb)
        return;

    tb->tbl->SetProgressValue(tb->hwnd, newProgress, 100);
}
