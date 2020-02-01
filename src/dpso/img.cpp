
#include "img.h"

#include <algorithm>
#include <cassert>
#include <cstdio>

#include "stb_image_resize.h"
#include "stb_image_resize_progress.h"

#include "progress_tracker.h"


namespace dpso {
namespace img {


static void resizeProgress(float progress, void* userData)
{
    auto* progressTracker = static_cast<ProgressTracker*>(userData);
    progressTracker->update(progress);
}


void resize(
    const std::uint8_t* src, int srcW, int srcH, int srcPitch,
    std::uint8_t* dst, int dstW, int dstH, int dstPitch,
    ProgressTracker* progressTracker)
{
    if (progressTracker)
        progressTracker->advanceJob();

    ProgressTracker localProgressTracker(1, progressTracker);
    localProgressTracker.start();

    stbirSetProgressFn(resizeProgress, &localProgressTracker);
    stbir_resize_uint8(
        src, srcW, srcH, srcPitch,
        dst, dstW, dstH, dstPitch,
        1);
    stbirSetProgressFn(nullptr, nullptr);

    localProgressTracker.finish();
}


static void hBoxBlur(
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

    progressTracker.advanceJob();

    const auto kernelSize = 1 + radius * 2;

    for (int y = 0; y < h; ++y) {
        const auto* srcRow = src + y * srcPitch;

        int sum = *srcRow * (radius + 1);
        for (int x = 1; x <= radius; ++x) {
            const auto* srcPx = srcRow + std::min(x, w - 1);
            sum += *srcPx;
        }

        auto* dstRow = dst + y * dstPitch;
        for (int x = 0; x < w; ++x) {
            auto* dstPx = dstRow + x;
            *dstPx = sum / kernelSize;

            const auto* addPx = (
                srcRow + std::min(x + radius + 1, w - 1));
            const auto* removePx = (
                srcRow + std::max(x - radius, 0));

            sum += *addPx - *removePx;
        }

        progressTracker.update(static_cast<float>(y) / h);
    }
}


static void vBoxBlur(
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

    progressTracker.advanceJob();

    const auto kernelSize = 1 + radius * 2;

    for (int x = 0; x < w; ++x) {
        const auto* srcCol = src + x;

        int sum = *srcCol * (radius + 1);
        for (int y = 1; y <= radius; ++y) {
            const auto* srcPx = (
                srcCol + std::min(y, h - 1) * srcPitch);
            sum += *srcPx;
        }

        auto* dstCol = dst + x;
        for (int y = 0; y < h; ++y) {
            auto* dstPx = dstCol + y * dstPitch;
            *dstPx = sum / kernelSize;

            const auto* addPx = (
                srcCol + std::min(y + radius + 1, h - 1) * srcPitch);
            const auto* removePx = (
                srcCol + std::max(y - radius, 0) * srcPitch);

            sum += *addPx - *removePx;
        }

        progressTracker.update(static_cast<float>(x) / w);
    }
}


static int getNumBoxBlurJobs(int numIters)
{
    const auto numSubpassesPerIter = 2;  // vertical + horizontal
    return numIters * numSubpassesPerIter;
}


// This version of boxBlur() advances the progressTracker
// getNumBoxBlurJobs() times. It's an implementation detail of both
// public boxBlur() and unsharpMask().
static void boxBlur(
    const std::uint8_t* src, int srcPitch,
    std::uint8_t* dst, int dstPitch,
    std::uint8_t* tmp, int tmpPitch,
    int w, int h, int radius,
    int numIters,
    ProgressTracker& progressTracker)
{
    if (w < 1 || h < 1 || radius < 1 || numIters < 1) {
        progressTracker.advanceJob(getNumBoxBlurJobs(numIters));
        return;
    }

    const auto* curSrc = src;
    auto curSrcPitch = srcPitch;

    for (int i = 0; i < numIters; ++i) {
        hBoxBlur(
            curSrc, curSrcPitch, tmp, tmpPitch, w, h, radius,
            progressTracker);
        vBoxBlur(
            tmp, tmpPitch, dst, dstPitch, w, h, radius,
            progressTracker);

        curSrc = dst;
        curSrcPitch = dstPitch;
    }
}


void boxBlur(
    const std::uint8_t* src, int srcPitch,
    std::uint8_t* dst, int dstPitch,
    std::uint8_t* tmp, int tmpPitch,
    int w, int h, int radius,
    int numIters,
    ProgressTracker* progressTracker)
{
    if (progressTracker)
        progressTracker->advanceJob();

    ProgressTracker localProgressTracker(
        getNumBoxBlurJobs(numIters), progressTracker);
    localProgressTracker.start();

    boxBlur(
        src, srcPitch,
        dst, dstPitch,
        tmp, tmpPitch,
        w, h, radius,
        numIters,
        localProgressTracker);

    localProgressTracker.finish();
}


static void unsharp(
    const std::uint8_t* src, int srcPitch,
    const std::uint8_t* blurred, int blurredPitch,
    std::uint8_t* dst, int dstPitch,
    int w, int h,
    float amount,
    ProgressTracker& progressTracker)
{
    assert(srcPitch >= w);
    assert(blurredPitch >= w);
    assert(dstPitch >= w);

    progressTracker.advanceJob();

    for (int y = 0; y < h; ++y) {
        const auto* srcPx = src + y * srcPitch;
        const auto* blurredPx = blurred + y * blurredPitch;
        auto* dstPx = dst + y * dstPitch;

        for (int x = 0; x < w; ++x) {
            const int diff = *srcPx - *blurredPx;
            int value = *srcPx + diff * amount;
            if (value < 0)
                value = 0;
            else if (value > 255)
                value = 255;

            *dstPx = value;

            ++srcPx;
            ++blurredPx;
            ++dstPx;
        }

        progressTracker.update(static_cast<float>(y) / h);
    }
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
//     an even kernel come from W3C Filter Effects Module, which was
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
    float amount,
    ProgressTracker* progressTracker)
{
    static const int numBlurIters = 2;
    static const auto numJobs = getNumBoxBlurJobs(numBlurIters) + 1;

    if (progressTracker)
        progressTracker->advanceJob();

    ProgressTracker localProgressTracker(numJobs, progressTracker);
    localProgressTracker.start();

    boxBlur(
        src, srcPitch,
        dst, dstPitch,
        tmp, tmpPitch,
        w, h,
        radius,
        numBlurIters,
        localProgressTracker);

    unsharp(
        src, srcPitch,
        dst, dstPitch,
        dst, dstPitch,
        w, h,
        amount,
        localProgressTracker);

    localProgressTracker.finish();
}


static void savePnm(
    const char* fileName,
    const std::uint8_t* data, int w, int h, int pitch,
    int bytesPerPixel)
{
    assert(data);
    assert(w > 0);
    assert(h > 0);
    assert(bytesPerPixel == 1 || bytesPerPixel == 3);

    auto* fp = std::fopen(fileName, "wb");
    if (!fp)
        return;

    std::fprintf(
        fp,
        "P%c\n%i %i\n255\n",
        bytesPerPixel == 1 ? '5' : '6', w, h);
    for (int y = 0; y < h; ++y)
        std::fwrite(data + y * pitch, 1, w * bytesPerPixel, fp);

    std::fclose(fp);
}


void savePgm(
    const char* fileName,
    const std::uint8_t* data, int w, int h, int pitch)
{
    savePnm(fileName, data, w, h, pitch, 1);
}


}
}
