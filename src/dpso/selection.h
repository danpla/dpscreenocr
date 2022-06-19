
/**
 * \file
 * Interactive on-screen selection
 */

#pragma once

#include "geometry_c.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get whether selection is enabled.
 *
 * Returns 1 if enabled, or 0 if disabled. The selection is disabled
 * by default.
 */
int dpsoGetSelectionIsEnabled(void);


/**
 * Set whether selection is enabled.
 *
 * An active selection shows the user a rectangle between the pivot
 * point (the position at the moment the selection was enabled) and
 * the current mouse position. The appearance of the selection depends
 * on the platform and implementation.
 *
 * Non-zero newSelectionIsEnabled enables the selection, 0 disables
 * it. Enabling the already active selection will not reset the pivot
 * point; to do this, you have to explicitly disable end enable it
 * back.
 */
void dpsoSetSelectionIsEnabled(int newSelectionIsEnabled);


/**
 * Get selection geometry.
 *
 * You can use this function even if selection is not active, in which
 * case it will return the geometry at the moment the selection was
 * disabled.
 */
void dpsoGetSelectionGeometry(DpsoRect* rect);


#ifdef __cplusplus
}
#endif
