
#pragma once


#if DPSO_FORCE_TIMING || !defined(NDEBUG)

#include "printf_fn.h"


namespace dpso::timing {


float getTime();
void report(float startTime, const char* fmt, ...) DPSO_PRINTF_FN(2);


}


#define DPSO_START_TIMING(name) \
    const float name ## TimingStartTime = dpso::timing::getTime();

#define DPSO_END_TIMING(name, ...) \
    dpso::timing::report(name ## TimingStartTime, __VA_ARGS__)


#else


#define DPSO_START_TIMING(name)
#define DPSO_END_TIMING(name, ...)


#endif
