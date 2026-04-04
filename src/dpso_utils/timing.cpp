#include "timing.h"


#if DPSO_FORCE_TIMING || !defined(NDEBUG)


#include <chrono>
#include <cstdio>
#include <ratio>

#include "str_stdio.h"


namespace dpso::timing {


float getTime()
{
    using Clock = std::chrono::steady_clock;
    using FloatMs = std::chrono::duration<float, std::milli>;

    return FloatMs{Clock::now().time_since_epoch()}.count();
}


void report(
    float startTime,
    std::string_view fmt,
    std::initializer_list<std::string_view> args)
{
    const auto duration = getTime() - startTime;

    str::print("Timing: ");
    str::print(fmt, args);
    str::print(": {} ms\n", duration);

    std::fflush(stdout);
}


}


#endif
