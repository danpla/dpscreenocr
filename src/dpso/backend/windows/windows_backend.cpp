
#include "backend/windows/windows_backend.h"

#include <windows.h>
#include "backend/null/null_screenshot.h"


namespace dpso {
namespace backend {


WindowsBackend::WindowsBackend()
{
    keyManager.reset(new WindowsKeyManager());
    selection.reset(new NullSelection());
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
    return new NullScreenshot(rect);
}


void WindowsBackend::update()
{
    keyManager->clearLastHotkeyAction();

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
