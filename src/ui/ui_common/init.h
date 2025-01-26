#pragma once

#include <stdbool.h>

#include "startup_args.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize the UI library.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool uiInit(int argc, char* argv[], UiStartupArgs* startupArgs);


#ifdef __cplusplus
}
#endif
