
#include "timing.h"


#if DPSO_FORCE_TIMING || !defined(NDEBUG)


#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ratio>


namespace dpso::timing {


float getTime()
{
    using Clock = std::chrono::steady_clock;
    using FloatMs = std::chrono::duration<float, std::milli>;

    return FloatMs{Clock::now().time_since_epoch()}.count();
}


void report(float startTime, const char* fmt, ...)
{
    const auto time = getTime() - startTime;

    std::printf("Timing: ");

    std::va_list args;
    va_start(args, fmt);
    std::vprintf(fmt, args);
    va_end(args);

    std::printf(": %f ms\n", time);

    std::fflush(stdout);
}


}


#endif
