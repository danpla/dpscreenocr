#pragma once

#include <memory>
#include <optional>
#include <type_traits>
#include <utility>


namespace dpso::backend::windows {


// BgThreadExecutor invokes callables in the same background thread.
//
// On Windows, each thread has its own message queue, so messages of
// the main thread will be consumed by the GUI framework and will not
// reach our library. BgThreadExecutor allows us to have our own
// independent thread and queue.
class BgThreadExecutor {
public:
    BgThreadExecutor();
    ~BgThreadExecutor();

    BgThreadExecutor(const BgThreadExecutor&) = delete;
    BgThreadExecutor& operator=(const BgThreadExecutor&) = delete;

    BgThreadExecutor(BgThreadExecutor&&) = delete;
    BgThreadExecutor& operator=(BgThreadExecutor&&) = delete;

    // isActive() returns true if called from within operator(). This
    // method can be useful for assertions when you call operator()
    // only for the topmost routine of a deep call hierarchy (nested
    // calls for operator() are allowed, but usually unnecessary).
    bool isActive() const;

    template<
        typename FnT,
        std::enable_if_t<
            std::is_void_v<std::invoke_result_t<FnT>>, int> = 0>
    void operator()(const FnT& fn)
    {
        struct FnTask : Task {
            explicit FnTask(const FnT& fn)
                : fn{fn}
            {
            }

            void execute() override
            {
                fn();
            }

            const FnT& fn;
        } task{fn};

        execute(task);
    }

    template<
        typename FnT,
        typename ResultT = std::invoke_result_t<FnT>,
        std::enable_if_t<!std::is_void_v<ResultT>, int> = 0>
    ResultT operator()(const FnT& fn)
    {
        struct FnTask : Task {
            explicit FnTask(const FnT& fn)
                : fn{fn}
            {
            }

            void execute() override
            {
                result.emplace(fn());
            }

            const FnT& fn;
            std::optional<ResultT> result;
        } task{fn};

        execute(task);
        return std::move(*task.result);
    }
private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    struct Task {
        virtual ~Task() = default;

        virtual void execute() = 0;
    };

    void execute(Task& task);
};


}
