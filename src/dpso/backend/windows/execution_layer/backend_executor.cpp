
#include "backend/windows/execution_layer/backend_executor.h"

#include "backend/screenshot.h"
#include "backend/windows/execution_layer/action_executor.h"
#include "backend/windows/execution_layer/key_manager_executor.h"
#include "backend/windows/execution_layer/selection_executor.h"


namespace dpso {
namespace backend {
namespace {


class BackendExecutor : public Backend {
public:

    explicit BackendExecutor(BackendCreatorFn creatorFn);
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


BackendExecutor::BackendExecutor(BackendCreatorFn creatorFn)
    : actionExecutor{createBgThreadActionExecutor()}
    , backend{execute(*actionExecutor, creatorFn)}
    , keyManagerExecutor{backend->getKeyManager(), *actionExecutor}
    , selectionExecutor{backend->getSelection(), *actionExecutor}
{
}


BackendExecutor::~BackendExecutor()
{
    execute(*actionExecutor, [this](){
        backend.reset();
    });
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
    return execute(*actionExecutor, [&](){
        return backend->takeScreenshot(rect);
    });
}


void BackendExecutor::update()
{
    execute(*actionExecutor, [this](){ backend->update(); });
}


std::unique_ptr<Backend> createBackendExecutor(
    BackendCreatorFn creatorFn)
{
    return std::unique_ptr<Backend>(new BackendExecutor(creatorFn));
}


}
}
