#include "backend/windows/execution_layer/backend_executor.h"

#include "backend/windows/execution_layer/bg_thread_executor.h"
#include "backend/windows/execution_layer/key_manager_executor.h"
#include "backend/windows/execution_layer/selection_executor.h"


namespace dpso::backend {
namespace {


class BackendExecutor : public Backend {
public:
    explicit BackendExecutor(BackendCreatorFn creatorFn);
    ~BackendExecutor();

    KeyManager& getKeyManager() override;
    Selection& getSelection() override;
    img::ImgUPtr takeScreenshot(const Rect& rect) override;

    void update() override;
private:
    BgThreadExecutor bgThreadExecutor;

    std::unique_ptr<Backend> backend;

    KeyManagerExecutor keyManagerExecutor;
    SelectionExecutor selectionExecutor;
};


}


BackendExecutor::BackendExecutor(BackendCreatorFn creatorFn)
    : backend{bgThreadExecutor(creatorFn)}
    , keyManagerExecutor{backend->getKeyManager(), bgThreadExecutor}
    , selectionExecutor{backend->getSelection(), bgThreadExecutor}
{
}


BackendExecutor::~BackendExecutor()
{
    bgThreadExecutor([&]{ backend.reset(); });
}


KeyManager& BackendExecutor::getKeyManager()
{
    return keyManagerExecutor;
}


Selection& BackendExecutor::getSelection()
{
    return selectionExecutor;
}


img::ImgUPtr BackendExecutor::takeScreenshot(const Rect& rect)
{
    return bgThreadExecutor(
        [&]{ return backend->takeScreenshot(rect); });
}


void BackendExecutor::update()
{
    bgThreadExecutor([&]{ backend->update(); });
}


std::unique_ptr<Backend> createBackendExecutor(
    BackendCreatorFn creatorFn)
{
    return std::make_unique<BackendExecutor>(creatorFn);
}


}
