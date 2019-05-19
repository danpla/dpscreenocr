
#include "backend/windows/windows_backend_impl.h"

#include <windows.h>

#include "backend/windows/windows_screenshot.h"


namespace dpso {
namespace backend {


WindowsBackendImpl::WindowsBackendImpl()
    : keyManager {}
    , selection {}
{
    keyManager.reset(new WindowsKeyManager());
    selection.reset(new WindowsSelection());
}


WindowsKeyManager& WindowsBackendImpl::getKeyManager()
{
    return *keyManager;
}


WindowsSelection& WindowsBackendImpl::getSelection()
{
    return *selection;
}


Screenshot* WindowsBackendImpl::takeScreenshot(const Rect& rect)
{
    return WindowsScreenshot::take(rect);
}


void WindowsBackendImpl::update()
{
    keyManager->clearLastHotkeyAction();
    // Update the selection before handling events so that we
    // resize and repaint it on this update() rather than on the next.
    selection->update();

    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_HOTKEY)
            keyManager->handleWmHotkey(msg);
        else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}


}
}
