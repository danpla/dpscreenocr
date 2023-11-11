
#include "backend/windows/execution_layer/backend_executor.h"

#include "backend/backend_error.h"
#include "backend/screenshot.h"
#include "backend/windows/execution_layer/key_manager_executor.h"
#include "backend/windows/execution_layer/selection_executor.h"


namespace dpso::backend {
namespace {


class BackendExecutor : public Backend {
public:
    BackendExecutor(
        std::unique_ptr<ActionExecutor> actionExecutor,
        BackendCreatorFn creatorFn);
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


BackendExecutor::BackendExecutor(
        std::unique_ptr<ActionExecutor> actionExecutor,
        BackendCreatorFn creatorFn)
    : actionExecutor{std::move(actionExecutor)}
    , backend{execute(*this->actionExecutor, creatorFn)}
    , keyManagerExecutor{
        backend->getKeyManager(), *this->actionExecutor}
    , selectionExecutor{
        backend->getSelection(), *this->actionExecutor}
{
}


BackendExecutor::~BackendExecutor()
{
    execute(*actionExecutor, [&]{ backend.reset(); });
}


KeyManager& BackendExecutor::getKeyManager()
{
    return keyManagerExecutor;
}


Selection& BackendExecutor::getSelection()
{
    return selectionExecutor;
}


std::unique_ptr<Screenshot> BackendExecutor::takeScreenshot(
    const Rect& rect)
{
    return execute(*actionExecutor, [&]{
        return backend->takeScreenshot(rect);
    });
}


void BackendExecutor::update()
{
    execute(*actionExecutor, [&]{ backend->update(); });
}


std::unique_ptr<Backend> createBackendExecutor(
    std::unique_ptr<ActionExecutor> actionExecutor,
    BackendCreatorFn creatorFn)
{
    if (!actionExecutor)
        throw BackendError("actionExecutor is null");

    return std::make_unique<BackendExecutor>(
        std::move(actionExecutor), creatorFn);
}


}
