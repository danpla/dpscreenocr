
#pragma once


#if DPSO_FORCE_TIMING || !defined(NDEBUG)

#include "str.h"


namespace dpso::timing {


float getTime();

void report(
    float startTime,
    const char* fmt,
    std::initializer_list<const char*> args);

template<typename... Args>
void report(float startTime, const char* fmt, const Args&... args)
{
    report(startTime, fmt, {str::formatArg::get(args)...});
}


}


#define DPSO_START_TIMING(name) \
    const float name ## TimingStartTime = dpso::timing::getTime();

#define DPSO_END_TIMING(name, fmt, ...) \
    dpso::timing::report(name ## TimingStartTime, fmt, __VA_ARGS__)


#else


#define DPSO_START_TIMING(name)
#define DPSO_END_TIMING(name, fmt, ...)


#endif
