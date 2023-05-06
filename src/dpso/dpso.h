
/**
 * \file
 * Main header + library initialization, shutdown, and update
 */

#pragma once

#include <stdbool.h>

#include "key_manager.h"
#include "ocr.h"
#include "ocr_engine.h"
#include "ocr_lang_manager.h"
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
