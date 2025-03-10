/**
 * \file
 * Error message handling for C APIs.
 */

#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get the last error message for the current thread.
 *
 * The message is only applicable when a function signals an error. By
 * convention, if a function doesn't explicitly mention dpsoGetError()
 * in its documentation, this implies that the function doesn't set an
 * error message on failure and dpsoGetError() should not be used.
 */
const char* dpsoGetError(void);


#ifdef __cplusplus
}
#endif
