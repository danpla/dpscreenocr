#pragma once

#include <X11/Xlib.h>

#include "dpso_img/img.h"


namespace dpso {


struct Rect;


namespace backend::x11 {


img::ImgUPtr takeScreenshot(Display* display, const Rect& rect);


}
}
