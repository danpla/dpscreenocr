#include "screenshot.h"

#include "dpso_utils/error_set.h"
#include "dpso_utils/geometry.h"

#include "backend/backend.h"
#include "backend/screenshot_error.h"
#include "dpso_sys_p.h"


DpsoImg* dpsoTakeScreenshot(DpsoSys* sys, const DpsoRect* rect)
{
    if (!sys) {
        dpso::setError("sys is null");
        return {};
    }

    if (!rect) {
        dpso::setError("rect is null");
        return {};
    }

    try {
        return sys->backend->takeScreenshot(
            dpso::Rect{*rect}).release();
    } catch (dpso::backend::ScreenshotError& e) {
        dpso::setError("Backend::takeScreenshot(): {}", e.what());
        return {};
    }
}
