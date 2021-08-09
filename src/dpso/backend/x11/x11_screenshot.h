
#pragma once

#include <memory>

#include <X11/Xlib.h>

#include "backend/screenshot.h"


namespace dpso {


struct Rect;


namespace backend {


class Screenshot;


std::unique_ptr<Screenshot> takeX11Screenshot(
    Display* display, const Rect& rect);


}
}
