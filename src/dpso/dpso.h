
/**
 * \file
 * Main header + library initialization, shutdown, and update
 *
 * You should always include dpso/dpso.h rather than individual
 * headers.
 */

#pragma once

#include <stdbool.h>

#include "delay.h"
#include "error.h"
#include "hotkeys.h"
#include "ocr.h"
#include "selection.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize dpso library.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool dpsoInit(void);


/**
 * Shut down dpso library.
 */
void dpsoShutdown(void);


/**
 * Update the library.
 *
 * dpsoUpdate() process system events, like mouse motion, key press,
 * etc., for hotkeys handling (hotkeys.h) and interactive selection
 * (selection.h). Call this function at a frequency close to the
 * monitor refresh rate, which is usually 60 times per second.
 */
void dpsoUpdate(void);


#ifdef __cplusplus
}
#endif
