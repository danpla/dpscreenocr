#pragma once

#include <stdbool.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Directory separator for the current platform.
 */
extern const char dpsoDirSeparator;


/**
 * Block the current thread for the given number of milliseconds.
 */
void dpsoSleep(int milliseconds);


/**
 * Run an executable.
 *
 * If supported by the platform, exePath may be just the name of the
 * executable (e.g., to look up in the PATH environment variable).
 *
 * The function blocks the caller's thread until the executable exits.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool dpsoExec(
    const char* exePath, const char* const args[], size_t numArgs);


#ifdef __cplusplus
}
#endif
