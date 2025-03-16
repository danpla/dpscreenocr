#pragma once

#include <cstdint>

#include "px_format.h"


namespace dpso {


class ProgressTracker;


namespace img {


template<typename T>
T getMaskRightShift(T mask)
{
    if (mask == 0)
        return 0;

    T shift{};
    for (; !(mask & 1); mask >>= 1)
        ++shift;

    return shift;
}


void toGray(
    const std::uint8_t* src, int srcPitch, DpsoPxFormat srcPxFormat,
    std::uint8_t* dst, int dstPitch,
    int w, int h);


void resize(
    const std::uint8_t* src, int srcW, int srcH, int srcPitch,
    std::uint8_t* dst, int dstW, int dstH, int dstPitch);


void unsharpMask(
    const std::uint8_t* src, int srcPitch,
    std::uint8_t* dst, int dstPitch,
    std::uint8_t* tmp, int tmpPitch,
    int w, int h,
    int radius,
    float amount,
    ProgressTracker* progressTracker = nullptr);


}
}
