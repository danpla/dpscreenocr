
#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize app directory paths.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool uiInitAppDirs(const char* argv0);


#ifdef __cplusplus
}
#endif
