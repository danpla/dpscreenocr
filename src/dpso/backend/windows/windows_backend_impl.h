
#pragma once

#include <windows.h>

#include "backend/backend.h"
#include "backend/windows/windows_key_manager.h"
#include "backend/windows/windows_selection.h"


namespace dpso {
namespace backend {


class WindowsBackendImpl : public Backend {
public:
    WindowsBackendImpl();

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
}
