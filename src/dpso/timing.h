
#pragma once


#if defined(DPSO_FORCE_TIMING) || !defined(NDEBUG)


#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ratio>


namespace dpso {


template<typename ClockT = std::chrono::steady_clock>
struct CodeTimer {
    typename ClockT::time_point start;

    CodeTimer()
        : start {ClockT::now()}
    {}

    void report(const char* fmt, ...) const
    {
        const auto end = ClockT::now();
        const std::chrono::duration<float, std::milli> ms = (
            end - start);

        std::printf("Timing: ");

        va_list args;
        va_start(args, fmt);
        std::vprintf(fmt, args);
        va_end(args);

        std::printf(": %f ms\n", ms.count());
    }
};


}


#define START_TIMING(name) \
    const dpso::CodeTimer<> name ## CodeTimer

#define END_TIMING(name, ...) \
    name ## CodeTimer.report(__VA_ARGS__)


#else


#define START_TIMING(name)
#define END_TIMING(name, ...)


#endif
