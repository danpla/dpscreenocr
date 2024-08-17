
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
bool uiInitAppDirs(void);


#ifdef __cplusplus
}
#endif
