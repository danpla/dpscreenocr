
#pragma once

#include <ctime>
#include <string>


namespace dpso {


#ifdef __GNUC__
    #define DPSO_STRFTIME_FN(N) \
        __attribute__((format(strftime, N, 0)))
#else
    #define DPSO_STRFTIME_FN(N)
#endif


std::string strftime(
    const char* fmt, const std::tm* time) DPSO_STRFTIME_FN(1);


}
