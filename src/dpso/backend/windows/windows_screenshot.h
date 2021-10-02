
#pragma once

#include <memory>

#include "backend/screenshot.h"


namespace dpso {


struct Rect;


namespace backend {


std::unique_ptr<Screenshot> takeWindowsScreenshot(const Rect& rect);


}
}
