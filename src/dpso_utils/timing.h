
#pragma once


#if DPSO_FORCE_TIMING || !defined(NDEBUG)

#include <fmt/core.h>


namespace dpso::timing {


float getTime();

void vReport(
    float startTime, fmt::string_view format, fmt::format_args args);

template<typename... T>
void report(
    float startTime, fmt::format_string<T...> format, T&&... args)
{
    vReport(startTime, format, fmt::make_format_args(args...));
}


}


#define DPSO_START_TIMING(name) \
    const float name ## TimingStartTime = dpso::timing::getTime();

#define DPSO_END_TIMING(name, format, ...) \
    dpso::timing::report(name ## TimingStartTime, format, __VA_ARGS__)


#else


#define DPSO_START_TIMING(name)
#define DPSO_END_TIMING(name, format, ...)


#endif
