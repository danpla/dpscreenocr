#include "backend/x11/screenshot.h"

#include <X11/Xutil.h>

#include "dpso_img/ops.h"

#include "dpso_utils/byte_order.h"
#include "dpso_utils/error_get.h"
#include "dpso_utils/geometry.h"
#include "dpso_utils/scope_exit.h"
#include "dpso_utils/str.h"

#include "backend/screenshot_error.h"


namespace dpso::backend::x11 {
namespace {


// The pixel type implied by XImage::*_mask.
using XPixel = unsigned long;


template<int bytesPerPx, ByteOrder byteOrder, typename CCTransformer>
void getRgbData(
    const XImage& image,
    std::uint8_t* buf,
    int pitch,
    CCTransformer ccTransformer)
{
    const auto rShift = img::getMaskRightShift(image.red_mask);
    const auto gShift = img::getMaskRightShift(image.green_mask);
    const auto bShift = img::getMaskRightShift(image.blue_mask);

    for (int y = 0; y < image.height; ++y) {
        const auto* srcRow =
            reinterpret_cast<const std::uint8_t*>(image.data)
            + image.bytes_per_line * y;
        auto* dstRow = buf + pitch * y;

        for (int x = 0; x < image.width; ++x) {
            XPixel px;
            load<byteOrder, bytesPerPx>(px, srcRow);
            srcRow += bytesPerPx;

            *dstRow++ = ccTransformer(
                (px & image.red_mask) >> rShift);
            *dstRow++ = ccTransformer(
                (px & image.green_mask) >> gShift);
            *dstRow++ = ccTransformer(
                (px & image.blue_mask) >> bShift);
        }
    }
}


template<int bytesPerPx, typename CCTransformer>
void getRgbData(
    const XImage& image,
    std::uint8_t* buf,
    int pitch,
    CCTransformer ccTransformer)
{
    if (image.byte_order == LSBFirst)
        getRgbData<bytesPerPx, ByteOrder::little>(
            image, buf, pitch, ccTransformer);
    else
        getRgbData<bytesPerPx, ByteOrder::big>(
            image, buf, pitch, ccTransformer);
}


void getRgbData(const XImage& image, std::uint8_t* buf, int pitch)
{
    if (image.depth != 24  // XRGB 8-8-8-8
            && image.depth != 30  // XRGB 2-10-10-10
            && image.depth != 32)  // ARGB 8-8-8-8
        // There is no point in supporting 16- and 8-bit formats,
        // since nobody uses them anymore.
        throw ScreenshotError{str::format(
            "Bit depth {} is not supported", image.depth)};

    if (image.bits_per_pixel != 32)
        throw ScreenshotError{str::format(
            "Unexpected number of bits per pixel {}",
            image.bits_per_pixel)};

    if (image.depth == 30)
        // XRGB 2-10-10-10
        getRgbData<4>(
            image, buf, pitch, [](XPixel c){ return c / 4; });
    else
        // 24 (XRGB 8-8-8-8) and 32 (ARGB 8-8-8-8)
        getRgbData<4>(image, buf, pitch, [](XPixel c){ return c; });
}


}


img::ImgUPtr takeScreenshot(Display* display, const Rect& rect)
{
    auto* screen = XDefaultScreenOfDisplay(display);
    const Rect screenRect{
        0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen)};
    const auto captureRect = getIntersection(rect, screenRect);
    if (isEmpty(captureRect))
        throw ScreenshotError{"Rect is outside screen bounds"};

    auto* image = XGetImage(
        display,
        XDefaultRootWindow(display),
        captureRect.x,
        captureRect.y,
        captureRect.w,
        captureRect.h,
        AllPlanes,
        ZPixmap);

    if (!image)
        throw ScreenshotError{"XGetImage() failed"};

    const ScopeExit destroyImage{[&]{ XDestroyImage(image); }};

    img::ImgUPtr result{
        dpsoImgCreate(
            DpsoPxFormatRgb, image->width, image->height, 0)};
    if (!result)
        throw ScreenshotError{str::format(
            "dpsoImgCreate(): {}", dpsoGetError())};

    getRgbData(
        *image,
        dpsoImgGetData(result.get()),
        dpsoImgGetPitch(result.get()));

    return result;
}


}
