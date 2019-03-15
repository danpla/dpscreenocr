
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Run an executable.
 *
 * arg becomes argv[1]. If wait is 1, the function will wait for the
 * executable to terminate, blocking the caller's thread.
 */
void dpsoExec(const char* exePath, const char* arg, int wait);


#ifdef __cplusplus
}
#endif
