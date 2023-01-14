
/**
 * \file
 * Type checking for custom printf-like functions.
 *
 * File provides DPSO_PRINTF_FN(N) macro, where N is 1-based position
 * of the format string.
 */

#pragma once


#ifdef __GNUC__
    #ifdef __MINGW32__
        #include <stdio.h>
        #define DPSO_PRINTF_FN(N) \
            __attribute__((format(__MINGW_PRINTF_FORMAT, N, N + 1)))
    #else
        #define DPSO_PRINTF_FN(N) \
            __attribute__((format(printf, N, N + 1)))
    #endif
#else
    #define DPSO_PRINTF_FN(N)
#endif
