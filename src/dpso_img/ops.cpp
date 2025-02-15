#include "ops.h"

#include <algorithm>
#include <cassert>

#include "stb_image_resize2.h"

#include "dpso_utils/progress_tracker.h"
#include "dpso_utils/str.h"
#include "dpso_utils/stream/file_stream.h"
#include "dpso_utils/stream/utils.h"


namespace dpso::img {


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
        convertRgb(0, 1, 2);
        break;
    case DpsoPxFormatBgr:
        convertRgb(2, 1, 0);
        break;
    case DpsoPxFormatRgba:
        convertRgb(0, 1, 2);
        break;
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


void resize(
    const std::uint8_t* src, int srcW, int srcH, int srcPitch,
    std::uint8_t* dst, int dstW, int dstH, int dstPitch)
{
    stbir_resize_uint8_linear(
        src, srcW, srcH, srcPitch,
        dst, dstW, dstH, dstPitch,
        STBIR_1CHANNEL);
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

    const auto kernelSize = 1 + radius * 2;

    for (int y = 0; y < h; ++y) {
        const auto* srcRow = src + y * srcPitch;

        auto sum = *srcRow * (radius + 1);
        for (int x = 1; x <= radius; ++x)
            sum += srcRow[std::min(x, w - 1)];

        auto* dstRow = dst + y * dstPitch;
        for (int x = 0; x < w; ++x) {
            dstRow[x] = sum / kernelSize;

            const auto addPx =
                srcRow[std::min(x + radius + 1, w - 1)];
            const auto removePx = srcRow[std::max(x - radius, 0)];

            sum += addPx - removePx;
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

    const auto kernelSize = 1 + radius * 2;

    for (int x = 0; x < w; ++x) {
        const auto* srcCol = src + x;

        auto sum = *srcCol * (radius + 1);
        for (int y = 1; y <= radius; ++y)
            sum += srcCol[std::min(y, h - 1) * srcPitch];

        auto* dstCol = dst + x;
        for (int y = 0; y < h; ++y) {
            dstCol[y * dstPitch] = sum / kernelSize;

            const auto addPx =
                srcCol[std::min(y + radius + 1, h - 1) * srcPitch];
            const auto removePx =
                srcCol[std::max(y - radius, 0) * srcPitch];

            sum += addPx - removePx;
        }

        progressTracker.update(static_cast<float>(x) / w);
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
    const auto numSubpassesPerIter = 2;  // vertical + horizontal
    const auto numJobs = numIters * numSubpassesPerIter;

    ProgressTracker localProgressTracker(numJobs, &progressTracker);

    if (w < 1 || h < 1 || radius < 1 || numIters < 1) {
        localProgressTracker.finish();
        return;
    }

    const auto* curSrc = src;
    auto curSrcPitch = srcPitch;

    for (int i = 0; i < numIters; ++i) {
        localProgressTracker.advanceJob();
        hBoxBlur(
            curSrc, curSrcPitch, tmp, tmpPitch, w, h, radius,
            localProgressTracker);

        localProgressTracker.advanceJob();
        vBoxBlur(
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
    float amount,
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

        for (int x = 0; x < w; ++x) {
            const auto diff = srcRow[x] - blurredRow[x];
            dstRow[x] = std::clamp(
                srcRow[x] + static_cast<int>(diff * amount), 0, 255);
        }

        localProgressTracker.update(static_cast<float>(y) / h);
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
    float amount,
    ProgressTracker* progressTracker)
{
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
        amount,
        localProgressTracker);

    localProgressTracker.finish();
}


void savePgm(
    const char* filePath,
    const std::uint8_t* data, int w, int h, int pitch)
{
    if (w < 1 || h < 1 || pitch < w)
        return;

    try {
        FileStream file{filePath, FileStream::Mode::write};
        write(file, str::format("P5\n{} {}\n255\n", w, h));

        for (int y = 0; y < h; ++y)
            file.write(data + y * pitch, w);
    } catch (std::runtime_error&) {  // os::Error, StreamError
    }
}


}
