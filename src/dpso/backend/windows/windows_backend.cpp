
#include "backend/windows/windows_backend.h"

#include <windows.h>

#include "backend/windows/windows_screenshot.h"


namespace dpso {
namespace backend {


WindowsBackend::WindowsBackend()
{
    keyManager.reset(new WindowsKeyManager());
    selection.reset(new WindowsSelection());
}


KeyManager& WindowsBackend::getKeyManager()
{
    return *keyManager;
}


Selection& WindowsBackend::getSelection()
{
    return *selection;
}


Screenshot* WindowsBackend::takeScreenshot(const Rect& rect)
{
    return WindowsScreenshot::take(rect);
}


void WindowsBackend::update()
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
