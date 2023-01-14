
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Run an executable.
 *
 * arg becomes argv[1]. The function blocks the caller's thread until
 * the executable exits.
 */
void dpsoExec(const char* exePath, const char* arg);


#ifdef __cplusplus
}
#endif
