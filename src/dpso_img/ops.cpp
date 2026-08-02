#include "ops.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

#include "dpso_utils/progress_tracker.h"


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

        for (int y{}; y < h; ++y) {
            const auto* srcRow = src + y * srcPitch;
            auto* dstRow = dst + y * dstPitch;

            for (int x{}; x < w; ++x) {
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
        w[3] = 0.5f * t3 - 0.5f * t2;
        w[1] = 1.0f - w[0] - w[2] - w[3];

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


void Upscaler::operator()(
    const std::uint8_t* src, int srcW, int srcH, int srcPitch,
    std::uint8_t* dst, int dstW, int dstH, int dstPitch)
{
    impl->scale(src, srcW, srcH, srcPitch, dst, dstW, dstH, dstPitch);
}


namespace {


class BoxBlur {
public:
    void operator()(
        const std::uint8_t* src, int srcPitch,
        std::uint8_t* dst, int dstPitch,
        std::uint8_t* tmp, int tmpPitch,
        int w, int h, int radius,
        int numIters,
        ProgressTracker& progressTracker);
private:
    // When computing the average value of the moving blur kernel, we
    // replace the division with multiplication using a UQ8.24 fixed
    // point format to help the compiler vectorize the loops. 24 is
    // the maximum number fraction bits (N) that fits in a 32-bit
    // unsigned integer:
    //
    // sum * reciprocal(kernelSize)
    // = (255 * kernelSize) * (2^N / kernelSize)
    // = 255 * 2^N
    //
    // Which, with N = 24, gives 4'278'190'080.
    using Fp24 = std::uint32_t;

    static Fp24 u8ToFp24(std::uint8_t i)
    {
        return Fp24{i} << 24;
    }

    static std::uint8_t fp24ToU8(Fp24 fp)
    {
        return fp >> 24;
    }

    std::vector<int> sums;

    static void hPass(
        const std::uint8_t* src, int srcPitch,
        std::uint8_t* dst, int dstPitch,
        int w, int h,
        int radius,
        ProgressTracker& progressTracker);

    void vPass(
        const std::uint8_t* src, int srcPitch,
        std::uint8_t* dst, int dstPitch,
        int w, int h,
        int radius,
        ProgressTracker& progressTracker);
};


}


void BoxBlur::operator()(
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

    for (int i{}; i < numIters; ++i) {
        localProgressTracker.advanceJob();
        hPass(
            curSrc, curSrcPitch, tmp, tmpPitch, w, h, radius,
            localProgressTracker);

        localProgressTracker.advanceJob();
        vPass(
            tmp, tmpPitch, dst, dstPitch, w, h, radius,
            localProgressTracker);

        curSrc = dst;
        curSrcPitch = dstPitch;
    }

    localProgressTracker.finish();
}


void BoxBlur::hPass(
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

    const auto kernelSizeRecip = u8ToFp24(1) / (1 + radius * 2);

    for (int y{}; y < h; ++y) {
        const auto* srcRow = src + y * srcPitch;
        auto* dstRow = dst + y * dstPitch;

        auto sum = srcRow[0] * (radius + 1);
        for (int i{1}; i <= radius; ++i)
            sum += srcRow[std::min(i, w - 1)];

        // To avoid calling min/max() for each pixel, we split the
        // line into 4 ordered ranges based on when the sliding window
        // "detaches" from the left edge of the line (at which point
        // we no longer need to clamp the subtracted pixel index) and
        // when it hits the right edge (at which point we need to
        // start clamping the added pixel index).

        const auto processRange =
        [&](int begin, int end, auto getSubIdx, auto getAddIdx)
        {
            for (auto x = begin; x < end; ++x) {
                dstRow[x] = fp24ToU8(sum * kernelSizeRecip);
                sum += srcRow[getAddIdx(x)] - srcRow[getSubIdx(x)];
            }
        };

        // [0, p1) - the window only touches the left edge of the line
        const auto p1 = std::clamp(w - radius - 1, 0, radius);
        // [p1, p2) - the window touches both edges
        const auto p2 = std::min(w, radius);
        // [p2, p3) - the window doesn't touch the edges
        const auto p3 = std::max(p2, w - radius - 1);
        // [p3, w) - the window only touches the right edge

        const auto clampedSubIdx = [](int) { return 0; };
        const auto subIdx = [=](int x) { return x - radius; };

        const auto clampedAddIdx = [=](int) { return w - 1; };
        const auto addIdx = [=](int x) { return x + radius + 1; };

        processRange(0, p1, clampedSubIdx, addIdx);
        processRange(p1, p2, clampedSubIdx, clampedAddIdx);
        processRange(p2, p3, subIdx, addIdx);
        processRange(p3, w, subIdx, clampedAddIdx);

        progressTracker.update(static_cast<float>(y + 1) / h);
    }
}


void BoxBlur::vPass(
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

    // For better cache locality, the vertical pass operates on the
    // entire rows, making the algorithm about 4 times faster than
    // the naive variant (similar to the horizontal pass) that
    // processes individual columns.

    sums.resize(w);

    // If the sums vector comes from outside the function (either as
    // a reference parameter or via the implicit "this"), accessing
    // its data via operator[] will prevent the compiler (at least
    // GCC) from vectorizing the loops due to a possible aliasing of
    // the vector storage with the dst pointer. Since C++ lacks a
    // standard "restrict" qualifier to make things explicit, it's
    // crucial to access the vector data via a local pointer variable
    // rather than directly via operator[].
    auto* sums = this->sums.data();

    const auto kernelSizeRecip = u8ToFp24(1) / (1 + radius * 2);

    const auto* row0 = src;
    for (int x{}; x < w; ++x)
        sums[x] = row0[x] * (radius + 1);

    for (int y{1}; y <= radius; ++y) {
        const auto* row = src + std::min(y, h - 1) * srcPitch;

        for (int x{}; x < w; ++x)
            sums[x] += row[x];
    }

    for (int y{}; y < h; ++y) {
        auto* dstRow = dst + y * dstPitch;

        const auto* addRow =
            src + std::min(y + radius + 1, h - 1) * srcPitch;
        const auto* subRow = src + std::max(y - radius, 0) * srcPitch;

        for (int x{}; x < w; ++x) {
            dstRow[x] = fp24ToU8(sums[x] * kernelSizeRecip);
            sums[x] += addRow[x] - subRow[x];
        }

        progressTracker.update(static_cast<float>(y + 1) / h);
    }
}


struct UnsharpMask::Impl {
    BoxBlur boxBlur;
};


UnsharpMask::UnsharpMask()
    : impl{std::make_unique<Impl>()}
{
}


UnsharpMask::~UnsharpMask() = default;


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

    for (int y{}; y < h; ++y) {
        const auto* srcRow = src + y * srcPitch;
        const auto* blurredRow = blurred + y * blurredPitch;
        auto* dstRow = dst + y * dstPitch;

        for (int x{}; x < w; ++x)
            dstRow[x] = std::clamp(
                srcRow[x] + (srcRow[x] - blurredRow[x]), 0, 255);

        localProgressTracker.update(static_cast<float>(y + 1) / h);
    }

    localProgressTracker.finish();
}


void UnsharpMask::operator()(
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

    impl->boxBlur(
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
