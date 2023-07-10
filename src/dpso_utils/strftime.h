
#pragma once

#include <ctime>
#include <string>


namespace dpso {


#if defined(__GNUC__) || defined(__clang__)
    /* Old MinGW versions don't define __MINGW_STRFTIME_FORMAT. */
    #if defined(__MINGW32__) && defined(__MINGW_STRFTIME_FORMAT)
        #define DPSO_STRFTIME_FN(N) \
            __attribute__((format(__MINGW_STRFTIME_FORMAT, N, 0)))
    #else
        #define DPSO_STRFTIME_FN(N) \
            __attribute__((format(strftime, N, 0)))
    #endif
#else
    #define DPSO_STRFTIME_FN(N)
#endif


std::string strftime(
    const char* fmt, const std::tm* time) DPSO_STRFTIME_FN(1);


}
