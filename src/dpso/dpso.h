
/**
 * \file
 * Main header + library initialization, shutdown, and update
 *
 * You should always include dpso/dpso.h rather then individual
 * headers.
 */

#pragma once

#include "hotkeys.h"
#include "ocr.h"
#include "selection.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize dpso library.
 *
 * Returns 1 on success, or 0 on failure. In the latter case, you can
 * get the error message with dpsoGetError().
 */
int dpsoInit(void);


/**
 * Get the error message after dpsoInit() failure.
 *
 * After successful dpsoInit(), the function returns an empty string.
 */
const char* dpsoGetError(void);


/**
 * Shutdown dpso library.
 */
void dpsoShutdown(void);


/**
 * Update the library.
 *
 * dpsoUpdate() process system events, like mouse motion, key press,
 * etc., which is necessary for hotkeys handling (hotkeys.h) and
 * interactive selection (selection.h). Call this function at a
 * frequency close to the monitor refresh rate, which is usually 60
 * times per second.
 */
void dpsoUpdate(void);


#ifdef __cplusplus
}
#endif
