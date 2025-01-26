#pragma once

#include <cstdint>


namespace dpso {


class ProgressTracker;


namespace img {


template<typename T>
T getMaskRightShift(T mask)
{
    if (mask == 0)
        return 0;

    T shift = 0;
    for (; !(mask & 1); mask >>= 1)
        ++shift;

    return shift;
}


inline std::uint8_t expandTo8Bit(std::uint8_t c, unsigned numBits)
{
    const auto maxC = (1u << numBits) - 1;
    return (c * 255 + maxC / 2) / maxC;
}


inline std::uint8_t rgbToGray(
    std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    return (r * 2126 + g * 7152 + b * 722) / 10000;
}


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


void savePgm(
    const char* filePath,
    const std::uint8_t* data, int w, int h, int pitch);


}
}
