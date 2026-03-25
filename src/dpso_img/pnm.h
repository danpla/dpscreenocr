#pragma once

#include <cstdint>
#include <string_view>

#include "px_format.h"


namespace dpso::img {


std::string_view getPnmExt(DpsoPxFormat pxFormat);


void savePnm(
    std::string_view filePath,
    DpsoPxFormat pxFormat,
    const std::uint8_t* data,
    int w,
    int h,
    int pitch);


}
