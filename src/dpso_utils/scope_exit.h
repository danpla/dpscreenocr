
#pragma once

#include <utility>


namespace dpso {


template<typename Fn>
class ScopeExit {
public:
    explicit ScopeExit(Fn&& fn)
        : fn{std::move(fn)}
    {
    }

    ~ScopeExit()
    {
        fn();
    }

    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;

    ScopeExit(ScopeExit&&) = delete;
    ScopeExit& operator=(ScopeExit&&) = delete;
private:
    Fn fn;
};


}
