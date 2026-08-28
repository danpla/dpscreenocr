#pragma once

#include <X11/Xlib.h>

#include "dpso_img/img.h"
#include "dpso_utils/geometry_fwd.h"


namespace dpso::backend::x11 {


img::ImgUPtr takeScreenshot(Display* display, const Rect& rect);


}
