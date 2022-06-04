
#include "ui_taskbar.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shobjidl.h>
#include <versionhelpers.h>

#include "dpso/backend/windows/utils.h"
#include "dpso/error.h"


struct UiTaskbar {
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

    return new UiTaskbar{tbl3, hwnd};
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

    tb->tbl3->SetProgressState(tb->hwnd, tbpFlag);
}


void uiTaskbarSetProgress(struct UiTaskbar* tb, int progress)
{
    if (!tb)
        return;

    tb->tbl3->SetProgressValue(tb->hwnd, progress, 100);
}
