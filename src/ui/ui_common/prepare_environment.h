
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Prepare environment before running the program.
 *
 * This function is mainly intended to set up environment variables.
 * Since it can restart the executable, it should be one of the very
 * first routines called from main().
 */
void uiPrepareEnvironment(char* argv[]);


#ifdef __cplusplus
}
#endif
