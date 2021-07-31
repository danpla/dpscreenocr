
#include "backend/windows/execution_layer/backend_executor.h"

#include "backend/screenshot.h"


namespace dpso {
namespace backend {


BackendExecutor::BackendExecutor(CreatorFn creatorFn)
    : actionExecutor{createBgThreadActionExecutor()}
    , backend{
        execute(*actionExecutor, [&creatorFn](){
            return creatorFn();
        })
    }
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
    // We can take screenshots form any thread.
    return backend->takeScreenshot(rect);
}


void BackendExecutor::update()
{
    execute(*actionExecutor, [this](){ backend->update(); });
}


}
}
