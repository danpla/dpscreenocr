
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
 * Get default selection border width.
 *
 * This is the width that is set by default for backends that support
 * a custom border width.
 */
int dpsoGetSelectionDefaultBorderWidth(void);


/**
 * Set selection border width.
 *
 * The function treats the given width as a value for the base (e.g.
 * 96) DPI. If supported by the backend, the final width can be scaled
 * proportionally to the actual DPI, which may be the physical DPI of
 * the display, the virtual DPI set via global GUI/font scale
 * settings, or a combination of both. Some backends may not support
 * changing the border width at all.
 *
 * Values < 1 will be clamped to 1.
 */
void dpsoSetSelectionBorderWidth(int newBorderWidth);


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
