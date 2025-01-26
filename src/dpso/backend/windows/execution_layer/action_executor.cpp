#include "backend/windows/execution_layer/action_executor.h"


namespace dpso::backend {


ActionExecutor::ActionExecutor()
    : thread{&ActionExecutor::threadLoop, this}
{
}


ActionExecutor::~ActionExecutor()
{
    backend::execute(*this, [&]{ terminate = true; });
    thread.join();
}


void ActionExecutor::execute(Action& action)
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


void ActionExecutor::threadLoop()
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


}
