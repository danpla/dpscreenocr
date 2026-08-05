#pragma once

#include <cstdint>
#include <memory>

#include "px_format.h"


namespace dpso::img {


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


// As the name implies, the class is designed for scaling images up.
// Scaling down is technically possible, but the quality will be poor.
class Upscale {
public:
    Upscale();
    ~Upscale();

    Upscale(const Upscale&) = delete;
    Upscale& operator=(const Upscale&) = delete;

    Upscale(Upscale&&) = delete;
    Upscale& operator=(Upscale&&) = delete;

    void operator()(
        const std::uint8_t* src, int srcW, int srcH, int srcPitch,
        std::uint8_t* dst, int dstW, int dstH, int dstPitch);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


class UnsharpMask {
public:
    UnsharpMask();
    ~UnsharpMask();

    UnsharpMask(const UnsharpMask&) = delete;
    UnsharpMask& operator=(const UnsharpMask&) = delete;

    UnsharpMask(UnsharpMask&&) = delete;
    UnsharpMask& operator=(UnsharpMask&&) = delete;

    void operator()(
        const std::uint8_t* src, int srcPitch,
        std::uint8_t* dst, int dstPitch,
        int w, int h,
        int radius);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


}
