#pragma once

#include <memory>

#include "dpso_img/img.h"


namespace dpso {


struct Rect;


namespace backend::windows {


img::ImgUPtr takeScreenshot(const Rect& rect);


}
}
