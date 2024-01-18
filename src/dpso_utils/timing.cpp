
#include "timing.h"


#if DPSO_FORCE_TIMING || !defined(NDEBUG)


#include <chrono>
#include <cstdio>
#include <ratio>


namespace dpso::timing {


float getTime()
{
    using Clock = std::chrono::steady_clock;
    using FloatMs = std::chrono::duration<float, std::milli>;

    return FloatMs{Clock::now().time_since_epoch()}.count();
}


void vReport(
    float startTime, fmt::string_view format, fmt::format_args args)
{
    const auto duration = getTime() - startTime;

    fmt::print("Timing: ");
    fmt::vprint(format, args);
    fmt::print(": {} ms\n", duration);

    std::fflush(stdout);
}


}


#endif
