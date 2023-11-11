
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
    std::condition_variable actionSetCondVar;
    std::condition_variable actionDoneCondVar;
    std::mutex mutex;

    bool terminate{};
    Action* currentAction{};
    std::exception_ptr actionException;

    void threadLoop();
};


}

BgThreadActionExecutor::BgThreadActionExecutor()
    : thread{std::thread(&BgThreadActionExecutor::threadLoop, this)}
{
}


BgThreadActionExecutor::~BgThreadActionExecutor()
{
    backend::execute(*this, [&]{ terminate = true; });
    thread.join();
}


void BgThreadActionExecutor::execute(Action& action)
{
    {
        const std::lock_guard guard{mutex};
        currentAction = &action;
    }

    actionSetCondVar.notify_one();

    {
        std::unique_lock lock{mutex};
        actionDoneCondVar.wait(lock, [&]{ return !currentAction; });
    }

    if (actionException)
        std::rethrow_exception(
            std::exchange(actionException, nullptr));
}


void BgThreadActionExecutor::threadLoop()
{
    // No need to protect "terminate" since it's set from within an
    // action.
    while (!terminate) {
        std::unique_lock lock{mutex};
        actionSetCondVar.wait(lock, [&]{ return currentAction; });

        try {
            currentAction->action();
        } catch (...) {
            actionException = std::current_exception();
        }

        currentAction = nullptr;

        // Unlock manually to avoid waking up the caller's thread only
        // to block again.
        lock.unlock();
        actionDoneCondVar.notify_one();
    }
}


std::unique_ptr<ActionExecutor> createBgThreadActionExecutor()
{
    return std::make_unique<BgThreadActionExecutor>();
}


}
