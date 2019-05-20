
#include "backend/windows/execution_layer/backend_executor.h"


namespace dpso {
namespace backend {


WindowsBackendExecutor::WindowsBackendExecutor()
    : actionExecutor {}
    , backend {}
    , keyManagerExecutor {}
    , selectionExecutor {}
{
    execute(actionExecutor, [this](){
        backend.reset(new WindowsBackendImpl());
    });

    keyManagerExecutor.reset(
        new KeyManagerExecutor(
            backend->getKeyManager(), actionExecutor));

    selectionExecutor.reset(
        new SelectionExecutor(
            backend->getSelection(), actionExecutor));
}


WindowsBackendExecutor::~WindowsBackendExecutor()
{
    execute(actionExecutor, [this](){
        backend.reset();
    });
}


KeyManagerExecutor& WindowsBackendExecutor::getKeyManager()
{
    return *keyManagerExecutor;
}


SelectionExecutor& WindowsBackendExecutor::getSelection()
{
    return *selectionExecutor;
}


WindowsScreenshot* WindowsBackendExecutor::takeScreenshot(
    const Rect& rect)
{
    // We can take screenshots form any thread.
    return backend->takeScreenshot(rect);
}


void WindowsBackendExecutor::update()
{
    execute(actionExecutor, [this](){ backend->update(); });
}


}
}
