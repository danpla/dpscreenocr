
#pragma once

#include <memory>

#include "backend/backend.h"
#include "backend/windows/windows_key_manager.h"
#include "backend/windows/windows_selection.h"


namespace dpso {
namespace backend {


class WindowsBackendImpl : public Backend {
public:
    WindowsBackendImpl();

    WindowsKeyManager& getKeyManager() override;
    WindowsSelection& getSelection() override;
    Screenshot* takeScreenshot(const Rect& rect) override;

    void update() override;
private:
    std::unique_ptr<WindowsKeyManager> keyManager;
    std::unique_ptr<WindowsSelection> selection;
};


}
}
