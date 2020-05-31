
#include "backend/windows/windows_backend_impl.h"

#include <string>

#include "backend/windows/windows_screenshot.h"
#include "utils.h"


namespace dpso {
namespace backend {


WindowsBackendImpl::WindowsBackendImpl()
    : instance {GetModuleHandleA(nullptr)}
    , keyManager {}
    , selection {}
{
    if (!instance)
        throw BackendError(
            "GetModuleHandle() failed: "
            + windows::getLastErrorMessage());

    try {
        keyManager.reset(new WindowsKeyManager());
    } catch (BackendError& e) {
        throw BackendError(
            std::string("Can't create key manager: ") + e.what());
    }

    try {
        selection.reset(new WindowsSelection(instance));
    } catch (BackendError& e) {
        throw BackendError(
            std::string("Can't create selection: ") + e.what());
    }
}


KeyManager& WindowsBackendImpl::getKeyManager()
{
    return *keyManager;
}


Selection& WindowsBackendImpl::getSelection()
{
    return *selection;
}


std::unique_ptr<Screenshot> WindowsBackendImpl::takeScreenshot(
    const Rect& rect)
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
