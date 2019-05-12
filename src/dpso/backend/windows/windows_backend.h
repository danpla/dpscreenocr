
#pragma once

#include <memory>

#include "backend/backend.h"
#include "backend/windows/windows_key_manager.h"
#include "backend/null/null_selection.h"


namespace dpso {
namespace backend {


class WindowsBackend : public Backend {
public:
    WindowsBackend();

    KeyManager& getKeyManager() override;
    Selection& getSelection() override;
    Screenshot* takeScreenshot(const Rect& rect) override;

    void update() override;
private:
    std::unique_ptr<WindowsKeyManager> keyManager;
    std::unique_ptr<NullSelection> selection;
};


}
}
