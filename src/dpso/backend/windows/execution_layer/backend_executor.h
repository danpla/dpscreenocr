
#pragma once

#include "backend/windows/execution_layer/action_executor.h"
#include "backend/windows/execution_layer/key_manager_executor.h"
#include "backend/windows/execution_layer/selection_executor.h"
#include "backend/windows/windows_backend_impl.h"


namespace dpso {
namespace backend {


class WindowsBackendExecutor : public Backend {
public:
    WindowsBackendExecutor();
    ~WindowsBackendExecutor();

    KeyManager& getKeyManager() override;
    Selection& getSelection() override;
    std::unique_ptr<Screenshot> takeScreenshot(
        const Rect& rect) override;

    void update() override;
private:
    BgThreadActionExecutor actionExecutor;

    std::unique_ptr<WindowsBackendImpl> backend;

    std::unique_ptr<KeyManagerExecutor> keyManagerExecutor;
    std::unique_ptr<SelectionExecutor> selectionExecutor;
};


}
}
