
#pragma once

#include "backend/backend.h"
#include "backend/windows/execution_layer/action_executor.h"
#include "backend/windows/execution_layer/key_manager_executor.h"
#include "backend/windows/execution_layer/selection_executor.h"


namespace dpso {
namespace backend {


class BackendExecutor : public Backend {
public:
    using CreatorFn = std::unique_ptr<Backend> (&)();

    explicit BackendExecutor(CreatorFn creatorFn);
    ~BackendExecutor();

    KeyManager& getKeyManager() override;
    Selection& getSelection() override;
    std::unique_ptr<Screenshot> takeScreenshot(
        const Rect& rect) override;

    void update() override;
private:
    std::unique_ptr<ActionExecutor> actionExecutor;

    std::unique_ptr<Backend> backend;

    KeyManagerExecutor keyManagerExecutor;
    SelectionExecutor selectionExecutor;
};


}
}
