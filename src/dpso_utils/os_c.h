
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Directory separators for the current platform.
 *
 * The primary separator is first in the list.
 */
extern const char* const dpsoDirSeparators;


/**
 * Block the current thread for the given number of milliseconds.
 */
void dpsoSleep(int milliseconds);


#ifdef __cplusplus
}
#endif
