#include "ops.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

#include "dpso_utils/progress_tracker.h"

#include "ops_utils.h"


namespace dpso::img {


static std::uint8_t rgbToGray(
    std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    return (r * 2126 + g * 7152 + b * 722) / 10000;
}


void toGray(
    const std::uint8_t* src, int srcPitch, DpsoPxFormat srcPxFormat,
    std::uint8_t* dst, int dstPitch,
    int w, int h)
{
    const auto convert = [&](auto grayExtractor)
    {
        const auto srcBpp = dpsoPxFormatGetBytesPerPx(srcPxFormat);

        for (int y = 0; y < h; ++y) {
            const auto* srcRow = src + y * srcPitch;
            auto* dstRow = dst + y * dstPitch;

            for (int x = 0; x < w; ++x) {
                dstRow[x] = grayExtractor(srcRow);
                srcRow += srcBpp;
            }
        }
    };

    const auto convertRgb = [&](int rIdx, int gIdx, int bIdx)
    {
        convert(
            [=](const std::uint8_t* px)
            {
                return rgbToGray(px[rIdx], px[gIdx], px[bIdx]);
            });
    };

    switch (srcPxFormat) {
    case DpsoPxFormatGrayscale:
        convert(
            [](const std::uint8_t* px)
            {
                return *px;
            });
        break;
    case DpsoPxFormatRgb:
    case DpsoPxFormatRgba:
        convertRgb(0, 1, 2);
        break;
    case DpsoPxFormatBgr:
    case DpsoPxFormatBgra:
        convertRgb(2, 1, 0);
        break;
    case DpsoPxFormatArgb:
        convertRgb(1, 2, 3);
        break;
    case DpsoPxFormatAbgr:
        convertRgb(3, 2, 1);
        break;
    }
}


struct Upscaler::Impl {
    struct Filter {
        static constexpr auto size{4};

        int idx[size];
        float weights[size];
    };

    std::vector<Filter> filters;
    std::vector<float> tmp;

    void fillFilters(int srcSize, int dstSize);
    void scale(
        const std::uint8_t* src, int srcW, int srcH, int srcPitch,
        std::uint8_t* dst, int dstW, int dstH, int dstPitch);
};


void Upscaler::Impl::fillFilters(int srcSize, int dstSize)
{
    filters.resize(dstSize);

    const auto scale = static_cast<float>(dstSize) / srcSize;

    const auto clamp = [=](int idx)
    {
        return std::clamp(idx, 0, srcSize - 1);
    };

    for (int i{}; i < dstSize; ++i) {
        const auto srcPos = (i + 0.5f) / scale - 0.5f;
        const auto centerIdx = static_cast<int>(
            std::floor(srcPos));

        auto& f = filters[i];

        const auto t = srcPos - centerIdx;
        const auto t2 = t * t;
        const auto t3 = t2 * t;

        auto* w = f.weights;

        // Catmull-Rom filter
        w[0] = -0.5f * t3 + t2 - 0.5f * t;
        w[2] = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
        w[3] =  0.5f * t3 - 0.5f * t2;
        w[1] =  1.0f - w[0] - w[2] - w[3];

        f.idx[0] = clamp(centerIdx - 1);
        f.idx[1] = clamp(centerIdx);
        f.idx[2] = clamp(centerIdx + 1);
        f.idx[3] = clamp(centerIdx + 2);
    }
}


void Upscaler::Impl::scale(
    const std::uint8_t* src, int srcW, int srcH, int srcPitch,
    std::uint8_t* dst, int dstW, int dstH, int dstPitch)
{
    tmp.resize(dstW * srcH);

    fillFilters(srcW, dstW);

    for (int y{}; y < srcH; ++y) {
        const auto* srcRow = src + y * srcPitch;
        auto* tmpRow = tmp.data() + y * dstW;

        for (int x{}; x < dstW; ++x) {
            const auto& filter = filters[x];
            auto sum = 0.0f;

            for (int k{}; k < Filter::size; ++k)
                sum += srcRow[filter.idx[k]] * filter.weights[k];

            tmpRow[x] = sum;
        }
    }

    fillFilters(srcH, dstH);

    for (int y{}; y < dstH; ++y) {
        const auto& filter = filters[y];
        auto* dstRow = dst + y * dstPitch;

        const float* tmpRows[Filter::size];
        for (int k{}; k < Filter::size; ++k)
            tmpRows[k] = tmp.data() + filter.idx[k] * dstW;

        for (int x{}; x < dstW; ++x) {
            // Start with 0.5 for rounding via cast at the end.
            auto sum = 0.5f;

            for (int k{}; k < Filter::size; ++k)
                sum += tmpRows[k][x] * filter.weights[k];

            dstRow[x] = std::clamp<int>(sum, 0, 255);
        }
    }
}


Upscaler::Upscaler()
    : impl{std::make_unique<Impl>()}
{
}


Upscaler::~Upscaler() = default;
Upscaler::Upscaler(Upscaler&&) noexcept = default;
Upscaler& Upscaler::operator=(Upscaler&&) noexcept = default;


void Upscaler::operator()(
    const std::uint8_t* src, int srcW, int srcH, int srcPitch,
    std::uint8_t* dst, int dstW, int dstH, int dstPitch)
{
    impl->scale(src, srcW, srcH, srcPitch, dst, dstW, dstH, dstPitch);
}


template<Axis axis>
static void boxBlur(
    const std::uint8_t* src, int srcPitch,
    std::uint8_t* dst, int dstPitch,
    int w, int h,
    int radius,
    ProgressTracker& progressTracker)
{
    assert(w > 0);
    assert(h > 0);
    assert(radius > 0);
    assert(srcPitch >= w);
    assert(dstPitch >= w);

    const auto kernelSize = 1 + radius * 2;

    const auto numLines = getSize<getOpposite(axis)>(w, h);
    const auto lineSize = getSize<axis>(w, h);

    for (int l = 0; l < numLines; ++l) {
        const auto srcLine = makeLine<axis>(l, src, srcPitch);

        auto sum = srcLine[0] * (radius + 1);
        for (int i = 1; i <= radius; ++i)
            sum += srcLine[std::min(i, lineSize - 1)];

        const auto dstLine = makeLine<axis>(l, dst, dstPitch);
        for (int i = 0; i < lineSize; ++i) {
            dstLine[i] = sum / kernelSize;

            const auto addPx =
                srcLine[std::min(i + radius + 1, lineSize - 1)];
            const auto removePx = srcLine[std::max(i - radius, 0)];

            sum += addPx - removePx;
        }

        progressTracker.update(static_cast<float>(l + 1) / numLines);
    }
}


static void boxBlur(
    const std::uint8_t* src, int srcPitch,
    std::uint8_t* dst, int dstPitch,
    std::uint8_t* tmp, int tmpPitch,
    int w, int h, int radius,
    int numIters,
    ProgressTracker& progressTracker)
{
    assert(srcPitch >= w);
    assert(dstPitch >= w);
    assert(tmpPitch >= w);
    assert(w > 0);
    assert(h > 0);
    assert(radius > 0);
    assert(numIters > 0);

    const auto numSubpassesPerIter = 2;  // vertical + horizontal
    const auto numJobs = numIters * numSubpassesPerIter;

    ProgressTracker localProgressTracker(numJobs, &progressTracker);

    const auto* curSrc = src;
    auto curSrcPitch = srcPitch;

    for (int i = 0; i < numIters; ++i) {
        localProgressTracker.advanceJob();
        boxBlur<Axis::x>(
            curSrc, curSrcPitch, tmp, tmpPitch, w, h, radius,
            localProgressTracker);

        localProgressTracker.advanceJob();
        boxBlur<Axis::y>(
            tmp, tmpPitch, dst, dstPitch, w, h, radius,
            localProgressTracker);

        curSrc = dst;
        curSrcPitch = dstPitch;
    }

    localProgressTracker.finish();
}


static void unsharp(
    const std::uint8_t* src, int srcPitch,
    const std::uint8_t* blurred, int blurredPitch,
    std::uint8_t* dst, int dstPitch,
    int w, int h,
    ProgressTracker& progressTracker)
{
    assert(srcPitch >= w);
    assert(blurredPitch >= w);
    assert(dstPitch >= w);

    ProgressTracker localProgressTracker(1, &progressTracker);
    localProgressTracker.advanceJob();

    for (int y = 0; y < h; ++y) {
        const auto* srcRow = src + y * srcPitch;
        const auto* blurredRow = blurred + y * blurredPitch;
        auto* dstRow = dst + y * dstPitch;

        for (int x = 0; x < w; ++x)
            dstRow[x] = std::clamp(
                srcRow[x] + (srcRow[x] - blurredRow[x]), 0, 255);

        localProgressTracker.update(static_cast<float>(y + 1) / h);
    }

    localProgressTracker.finish();
}


// Compared to the implementation in GIMP 2.8, our unsharp mask has
// several simplifications for our particular use case. They don't
// affect OCR quality, but improve performance or simplify the code.
//
//   * We don't use Gaussian blur instead of box if radius is < 10.
//
//   * We use 2 box blur iterations instead of 3.
//
//   * We don't try to approximate the Gaussian kernel width for box,
//     so our kernel is always odd.
//
//     The formula for approximation and instructions on how to apply
//     an even kernel comes from W3C Filter Effects Module, which was
//     previously part of the SVG specification:
//
//         https://www.w3.org/TR/filter-effects-1
//
//   * We don't need thresholding.
//
// The GIMP implementation is in /plug-ins/common/unsharp-mask.c,
// Git branch gimp-2-8.
void unsharpMask(
    const std::uint8_t* src, int srcPitch,
    std::uint8_t* dst, int dstPitch,
    std::uint8_t* tmp, int tmpPitch,
    int w, int h,
    int radius,
    ProgressTracker* progressTracker)
{
    if (srcPitch < w
            || dstPitch < w
            || tmpPitch < w
            || w < 1
            || h < 1
            || radius < 1)
        return;

    ProgressTracker localProgressTracker(2, progressTracker);

    localProgressTracker.advanceJob();
    boxBlur(
        src, srcPitch,
        dst, dstPitch,
        tmp, tmpPitch,
        w, h,
        radius,
        2,
        localProgressTracker);

    localProgressTracker.advanceJob();
    unsharp(
        src, srcPitch,
        dst, dstPitch,
        dst, dstPitch,
        w, h,
        localProgressTracker);

    localProgressTracker.finish();
}


}
