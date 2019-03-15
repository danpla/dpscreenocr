
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Run an executable.
 *
 * arg becomes argv[1]. If waitToComplete is 1, the function will wait
 * for the executable to complete, blocking the caller's thread.
 */
void dpsoExec(
    const char* exePath, const char* arg, int waitToComplete);


#ifdef __cplusplus
}
#endif
