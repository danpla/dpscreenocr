
#include "backend/windows/execution_layer/action_executor.h"

#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>


namespace dpso::backend {
namespace {


class BgThreadActionExecutor : public ActionExecutor {
public:
    BgThreadActionExecutor();
    ~BgThreadActionExecutor();

    void execute(Action& action) override;
private:
    std::thread thread;
    std::condition_variable condVar;
    std::mutex mutex;

    bool terminate;
    Action* currentAction;
    std::exception_ptr actionException;

    void threadLoop();
};


}

BgThreadActionExecutor::BgThreadActionExecutor()
    : thread{}
    , condVar{}
    , mutex{}
    , terminate{}
    , currentAction{}
    , actionException{}
{
    thread = std::thread(&BgThreadActionExecutor::threadLoop, this);
}


BgThreadActionExecutor::~BgThreadActionExecutor()
{
    dpso::backend::execute(*this, [this](){ terminate = true; });
    thread.join();
}


void BgThreadActionExecutor::execute(Action& action)
{
    {
        std::lock_guard guard{mutex};
        actionException = nullptr;
        currentAction = &action;
    }

    condVar.notify_one();

    {
        std::unique_lock lock{mutex};
        while (currentAction)
            condVar.wait(lock);
    }

    if (actionException)
        std::rethrow_exception(actionException);
}


void BgThreadActionExecutor::threadLoop()
{
    while (!terminate) {
        std::unique_lock lock{mutex};
        while (!currentAction)
            condVar.wait(lock);

        try {
            currentAction->action();
        } catch (...) {
            actionException = std::current_exception();
        }

        currentAction = nullptr;

        // Unlock manually to avoid waking up the caller's thread only
        // to block again.
        lock.unlock();
        condVar.notify_one();
    }
}


std::unique_ptr<ActionExecutor> createBgThreadActionExecutor()
{
    return std::make_unique<BgThreadActionExecutor>();
}


}
