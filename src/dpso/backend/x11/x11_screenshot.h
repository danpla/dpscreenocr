
#pragma once

#include <memory>

#include <X11/Xlib.h>

#include "backend/screenshot.h"


namespace dpso {


class Rect;


namespace backend {


class Screenshot;


std::unique_ptr<Screenshot> takeX11Screenshot(
    Display* display, const Rect& rect);


}
}
