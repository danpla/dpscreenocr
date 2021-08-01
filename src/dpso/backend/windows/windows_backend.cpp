
#include <string>

#include <windows.h>

#include "backend/backend.h"
#include "backend/backend_error.h"
#include "backend/windows/execution_layer/backend_executor.h"
#include "backend/windows/utils.h"
#include "backend/windows/windows_key_manager.h"
#include "backend/windows/windows_screenshot.h"
#include "backend/windows/windows_selection.h"


namespace dpso {
namespace backend {
namespace {


class WindowsBackend : public Backend {
public:
    WindowsBackend();

    KeyManager& getKeyManager() override;
    Selection& getSelection() override;
    std::unique_ptr<Screenshot> takeScreenshot(
        const Rect& rect) override;

    void update() override;
private:
    HINSTANCE instance;

    std::unique_ptr<WindowsKeyManager> keyManager;
    std::unique_ptr<WindowsSelection> selection;
};


}


WindowsBackend::WindowsBackend()
    : instance{GetModuleHandleA(nullptr)}
    , keyManager{}
    , selection{}
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


KeyManager& WindowsBackend::getKeyManager()
{
    return *keyManager;
}


Selection& WindowsBackend::getSelection()
{
    return *selection;
}


std::unique_ptr<Screenshot> WindowsBackend::takeScreenshot(
    const Rect& rect)
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


// We can't do anything in the main thread because its messages will
// be handed by a GUI framework. Of course we can provide a low-level
// routine to connect to an event filter provided by the framework,
// but some frameworks may not provide such filters, and I'd like to
// keep the API as high-level as possible.
//
// BackendExecutor and its proxy components do the job of handling the
// backend implementation in the background thread. Please note that
// although absolutely everything is called trough an executor, the
// only routines that must be called in the background are the ones
// that rely on Windows message queue; others don't technically need
// this.

std::unique_ptr<Backend> Backend::create()
{
    return createBackendExecutor(
        *[]()
        {
            return std::unique_ptr<Backend>(new WindowsBackend());
        });
}

}
}
