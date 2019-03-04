
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Run an executable.
 *
 * arg1 becomes argv[1].
 */
void dpsoExec(const char* exePath, const char* arg1);


#ifdef __cplusplus
}
#endif
