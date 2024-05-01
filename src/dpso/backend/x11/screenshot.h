
#pragma once

#include <memory>

#include <X11/Xlib.h>

#include "backend/screenshot.h"


namespace dpso {


struct Rect;


namespace backend::x11 {


std::unique_ptr<Screenshot> takeScreenshot(
    Display* display, const Rect& rect);


}
}
