
#pragma once

#include <memory>
#include <type_traits>
#include <utility>


namespace dpso::backend {


struct Action {
    virtual ~Action() = default;

    virtual void action() = 0;
};


class ActionExecutor {
public:
    virtual ~ActionExecutor() = default;

    virtual void execute(Action& action) = 0;
};


std::unique_ptr<ActionExecutor> createBgThreadActionExecutor();


// execute() handles return values manually to avoid heap allocations
// made by std::promise/future.


// Overload for callables that return nothing.
template<typename CallableT>
auto execute(ActionExecutor& executor, const CallableT& callable)
    -> std::enable_if_t<std::is_void_v<decltype(callable())>>
{
    struct CallableAction : Action {
        explicit CallableAction(const CallableT& callable)
            : callable{callable}
        {
        }

        void action() override
        {
            callable();
        }

        const CallableT& callable;
    } action(callable);

    executor.execute(action);
}


// Overload for callables that return a value.
template<typename CallableT>
auto execute(ActionExecutor& executor, const CallableT& callable)
    -> std::enable_if_t<
        !std::is_void_v<decltype(callable())>, decltype(callable())>
{
    struct CallableAction : Action {
        using ResultT = decltype(callable());

        explicit CallableAction(const CallableT& callable)
            : callable{callable}
            , result{}
        {
        }

        ~CallableAction()
        {
            if (result)
                result->~ResultT();
        }

        void action() override
        {
            result = new(resultMem) ResultT{callable()};
        }

        const CallableT& callable;
        alignas(ResultT) unsigned char resultMem[sizeof(ResultT)];
        ResultT* result;
    } action(callable);

    executor.execute(action);
    return std::move(*action.result);
}


}
