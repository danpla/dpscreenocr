
#pragma once

#include <stdbool.h>

#include "dpso_utils/geometry_c.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get whether selection is enabled.
 *
 * The selection is disabled by default.
 */
bool dpsoSelectionGetIsEnabled(void);


/**
 * Set whether selection is enabled.
 *
 * An active selection shows the user a rectangle between the anchor
 * point (the position at the moment the selection was enabled) and
 * the current mouse position. The appearance of the selection depends
 * on the platform and implementation.
 *
 * Enabling the already active selection will not reset the anchor
 * point; to do this, you have to explicitly disable end enable it
 * back.
 */
void dpsoSelectionSetIsEnabled(bool newIsEnabled);


/**
 * Get default selection border width.
 *
 * This is the width that is set by default for backends that support
 * a custom border width.
 */
int dpsoSelectionGetDefaultBorderWidth(void);


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
void dpsoSelectionSetBorderWidth(int newBorderWidth);


/**
 * Get selection geometry.
 *
 * You can use this function even if selection is not active, in which
 * case it will return the geometry at the moment the selection was
 * disabled.
 */
void dpsoSelectionGetGeometry(DpsoRect* rect);


#ifdef __cplusplus
}
#endif
