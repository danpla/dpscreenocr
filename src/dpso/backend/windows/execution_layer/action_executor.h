
#pragma once

#include <memory>
#include <type_traits>
#include <utility>


namespace dpso {
namespace backend {


struct Action {
    Action() = default;
    virtual ~Action() = default;

    Action(const Action& other) = delete;
    Action& operator=(const Action& other) = delete;

    Action(Action&& other) = delete;
    Action& operator=(Action&& other) = delete;

    virtual void action() = 0;
};


class ActionExecutor {
public:
    ActionExecutor() = default;
    virtual ~ActionExecutor() = default;

    ActionExecutor(const ActionExecutor& other) = delete;
    ActionExecutor& operator=(const ActionExecutor& other) = delete;

    ActionExecutor(ActionExecutor&& other) = delete;
    ActionExecutor& operator=(ActionExecutor&& other) = delete;

    virtual void execute(Action& action) = 0;
};


std::unique_ptr<ActionExecutor> createBgThreadActionExecutor();


// Overload for callables that return nothing.
template<typename CallableT>
auto execute(ActionExecutor& executor, CallableT callable)
    -> typename std::enable_if<std::is_void<decltype(callable())>::value>::type
{
    struct CallableAction : Action {
        explicit CallableAction(CallableT callable)
            : callable{callable}
        {
        }

        void action() override
        {
            callable();
        }

        CallableT callable;
    } action(callable);

    executor.execute(action);
}


// Overload for callables that return a value.
template<typename CallableT>
auto execute(ActionExecutor& executor, CallableT callable)
    -> typename std::enable_if<!std::is_void<decltype(callable())>::value, decltype(callable())>::type
{
    struct CallableAction : Action {
        explicit CallableAction(CallableT callable)
            : callable{callable}
        {
        }

        void action() override
        {
            result = callable();
        }

        CallableT callable;
        decltype(callable()) result;
    } action(callable);

    executor.execute(action);
    return std::move(action.result);
}


}
}
