
/**
 * \file
 * Error message handling.
 */

#pragma once

#ifdef __GNUC__
    #ifdef __MINGW32__
        #include <stdio.h>
        #define PRINTF_FORMAT __MINGW_PRINTF_FORMAT
    #else
        #define PRINTF_FORMAT printf
    #endif

    #define PRINTF_FN __attribute__((format(PRINTF_FORMAT, 1, 2)))
#else
    #define PRINTF_FN
#endif


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get the last error message.
 *
 * The message is only applicable when a function signals an error. By
 * convention, if a function doesn't explicitly mention dpsoGetError()
 * in its documentation, this implies that the function doesn't set an
 * error message on failure and dpsoGetError() should not be used.
 */
const char* dpsoGetError(void);


/**
 * Set error message.
 */
void dpsoSetError(const char* fmt, ...) PRINTF_FN;


#ifdef __cplusplus
}
#endif
