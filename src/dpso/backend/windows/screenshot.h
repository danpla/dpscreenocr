
#pragma once

#include <memory>

#include "backend/screenshot.h"


namespace dpso {


struct Rect;


namespace backend::windows {


std::unique_ptr<Screenshot> takeScreenshot(const Rect& rect);


}
}
