
#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize the UI library.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool uiInit(int argc, char* argv[]);


#ifdef __cplusplus
}
#endif
