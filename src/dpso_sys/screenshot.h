#pragma once

#include "dpso_img/img.h"
#include "dpso_utils/geometry_c.h"

#include "dpso_sys_fwd.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Take a screenshot.
 *
 * The screenshot is be taken from an intersection of the given rect
 * with the actual screen geometry.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * null.
 */
DpsoImg* dpsoTakeScreenshot(DpsoSys* sys, const DpsoRect* rect);


#ifdef __cplusplus
}
#endif
