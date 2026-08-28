#pragma once

#include "dpso_img/img.h"
#include "dpso_utils/geometry_fwd.h"


namespace dpso::backend::windows {


img::ImgUPtr takeScreenshot(const Rect& rect);


}
