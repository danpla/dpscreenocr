#include "backend/windows/bg_thread_executor.h"

#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>


namespace dpso::backend::windows {


struct BgThreadExecutor::Impl {
    Impl()
        : thread{&threadLoop, this}
    {
    }

    ~Impl()
    {
        {
            const std::lock_guard guard{mutex};
            terminateThread = true;
        }

        threadActionCondVar.notify_one();
        thread.join();
    }

    std::condition_variable threadActionCondVar;
    std::condition_variable taskDoneCondVar;
    std::mutex mutex;

    bool isInThread{};
    bool terminateThread{};
    Task* currentTask{};
    std::exception_ptr taskException;

    std::thread thread;

    void threadLoop();
    void execute(Task& task);
};


void BgThreadExecutor::Impl::threadLoop()
{
    while (true) {
        std::unique_lock lock{mutex};
        threadActionCondVar.wait(
            lock, [&]{ return terminateThread || currentTask; });

        if (terminateThread)
            break;

        try {
            currentTask->execute();
        } catch (...) {
            taskException = std::current_exception();
        }

        currentTask = {};

        // Unlock manually to avoid waking up the caller's thread only
        // to block again.
        lock.unlock();
        taskDoneCondVar.notify_one();
    }
}


void BgThreadExecutor::Impl::execute(Task& task)
{
    if (isInThread) {
        task.execute();
        return;
    }

    isInThread = true;

    {
        const std::lock_guard guard{mutex};
        currentTask = &task;
    }

    threadActionCondVar.notify_one();

    {
        std::unique_lock lock{mutex};
        taskDoneCondVar.wait(lock, [&]{ return !currentTask; });
    }

    isInThread = false;

    if (taskException)
        std::rethrow_exception(std::exchange(taskException, {}));
}


BgThreadExecutor::BgThreadExecutor()
    : impl{std::make_unique<Impl>()}
{
}


BgThreadExecutor::~BgThreadExecutor() = default;


bool BgThreadExecutor::isActive() const
{
    return impl->isInThread;
}


void BgThreadExecutor::execute(Task& task)
{
    impl->execute(task);
}


}
