#pragma once

#include <cstdint>

#include "px_format.h"


namespace dpso::img {


const char* getPnmExt(DpsoPxFormat pxFormat);


void savePnm(
    const char* filePath,
    DpsoPxFormat pxFormat,
    const std::uint8_t* data,
    int w,
    int h,
    int pitch);


}
