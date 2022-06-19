
#pragma once


#if DPSO_FORCE_TIMING || !defined(NDEBUG)

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ratio>

#include "printf_fn.h"


namespace dpso {


struct CodeTimer {
    using Clock = std::chrono::steady_clock;

    Clock::time_point start;

    CodeTimer()
        : start{Clock::now()}
    {}

    void report(const char* fmt, ...) const DPSO_PRINTF_FN(2)
    {
        const auto end = Clock::now();
        const std::chrono::duration<float, std::milli> ms =
            end - start;

        std::printf("Timing: ");

        std::va_list args;
        va_start(args, fmt);
        std::vprintf(fmt, args);
        va_end(args);

        std::printf(": %f ms\n", ms.count());

        std::fflush(stdout);
    }
};


}


#define START_TIMING(name) \
    const dpso::CodeTimer name ## CodeTimer

#define END_TIMING(name, ...) \
    name ## CodeTimer.report(__VA_ARGS__)


#else


#define START_TIMING(name)
#define END_TIMING(name, ...)


#endif
