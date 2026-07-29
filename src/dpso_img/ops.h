#pragma once

#include <cstdint>
#include <memory>

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


// As the name implies, the class is designed for scaling images up.
// Scaling down is technically possible, but the quality will be poor.
class Upscaler {
public:
    Upscaler();
    ~Upscaler();

    Upscaler(const Upscaler&) = delete;
    Upscaler& operator=(const Upscaler&) = delete;

    Upscaler(Upscaler&&) = delete;
    Upscaler& operator=(Upscaler&&) = delete;

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
        std::uint8_t* tmp, int tmpPitch,
        int w, int h,
        int radius,
        ProgressTracker* progressTracker = nullptr);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


}
}
