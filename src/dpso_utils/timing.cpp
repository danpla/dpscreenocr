#include "timing.h"


#if DPSO_FORCE_TIMING || !defined(NDEBUG)


#include <chrono>
#include <cstdio>
#include <ratio>

#include "str.h"


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

    std::fputs("Timing: ", stdout);
    std::fputs(str::format(fmt, args).c_str(), stdout);
    std::fputs(str::format(": {} ms\n", duration).c_str(), stdout);

    std::fflush(stdout);
}


}


#endif
