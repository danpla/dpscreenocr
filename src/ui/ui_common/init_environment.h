
#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Prepare the environment before running the program.
 *
 * This function is mainly intended to set up environment variables.
 * Since it can restart the executable, it should be one of the very
 * first routines called from main().
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool uiInitEnvironment(char* argv[]);


#ifdef __cplusplus
}
#endif
