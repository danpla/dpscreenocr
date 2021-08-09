
#include "backend/x11/x11_screenshot.h"

#include <cassert>

#include <X11/Xutil.h>

#include "img.h"


// We support the following XImage depths:
//   * 32 (ARGB 8-8-8-8)
//   * 30 (RGB 2-10-10-10)
//   * 24 (XRGB 8-8-8-8)
//   * 16 (RGB 5-6-5)
//   * 15 (RGB 5-5-5)
//
// My system also supports 1, 4, and 8, but with 8 most apps
// don't display colors correctly, so it's safe to assume that
// such depths are not common nowadays.
//
// We don't use SHM, at least for now. It requires the fixed image
// size, so it's necessary to read the whole screen to get an area
// of arbitrary size. Of course SHM is faster than XGetImage()
// when reading the whole screen (2 times faster on my machine with
// 1600x900 monitor: 10 vs 20 ms), but XGetImage() is faster for
// capturing small areas, which is more common in our case.


namespace dpso {
namespace backend {


std::unique_ptr<X11Screenshot> X11Screenshot::take(
    Display* display, const Rect& rect)
{
    auto* screen = XDefaultScreenOfDisplay(display);
    const Rect screenRect{
        0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen)
    };
    const auto captureRect = getIntersection(rect, screenRect);
    if (isEmpty(captureRect))
        return nullptr;

    auto* image = XGetImage(
        display,
        XDefaultRootWindow(display),
        captureRect.x,
        captureRect.y,
        captureRect.w,
        captureRect.h,
        AllPlanes,
        ZPixmap);

    if (image)
        return std::unique_ptr<X11Screenshot>(
            new X11Screenshot(image));
    else
        return nullptr;
}


X11Screenshot::X11Screenshot(XImage* image)
    : image{image}
{
    assert(image);
    assert(image->width > 0);
    assert(image->height > 0);
}


X11Screenshot::~X11Screenshot()
{
    XDestroyImage(image);
}


int X11Screenshot::getWidth() const
{
    return image->width;
}


int X11Screenshot::getHeight() const
{
    return image->height;
}


void X11Screenshot::getGrayscaleData(std::uint8_t* buf, int pitch) const
{
    auto rShift = img::getMaskRightShift(image->red_mask);
    auto gShift = img::getMaskRightShift(image->green_mask);
    auto bShift = img::getMaskRightShift(image->blue_mask);

    if (image->bits_per_pixel == 32) {
        const auto cDiv = image->depth == 30 ? 4 : 1;

        for (int y = 0; y < image->height; ++y) {
            const auto* srcRow = (
                reinterpret_cast<std::uint8_t*>(image->data)
                + image->bytes_per_line * y);
            auto* dstRow = buf + pitch * y;

            for (int x = 0; x < image->width; ++x) {
                std::uint32_t px;
                if (image->byte_order == LSBFirst)
                    px = (
                        static_cast<std::uint32_t>(srcRow[3]) << 24
                        | static_cast<std::uint32_t>(srcRow[2]) << 16
                        | static_cast<std::uint32_t>(srcRow[1]) << 8
                        | srcRow[0]);
                else
                    px = (
                        static_cast<std::uint32_t>(srcRow[0]) << 24
                        | static_cast<std::uint32_t>(srcRow[1]) << 16
                        | static_cast<std::uint32_t>(srcRow[2]) << 8
                        | srcRow[3]);
                srcRow += 4;

                const auto r = (
                    ((px & image->red_mask) >> rShift) / cDiv);
                const auto g = (
                    ((px & image->green_mask) >> gShift) / cDiv);
                const auto b = (
                    ((px & image->blue_mask) >> bShift) / cDiv);

                dstRow[x] = img::rgbToGray(r, g, b);
            }
        }
    } else if (image->bits_per_pixel == 16) {
        static const auto rBits = 5;
        const auto gBits = image->depth == 16 ? 6 : 5;
        static const auto bBits = 5;

        for (int y = 0; y < image->height; ++y) {
            const auto* srcRow = (
                reinterpret_cast<std::uint8_t*>(image->data)
                + image->bytes_per_line * y);
            auto* dstRow = buf + pitch * y;

            for (int x = 0; x < image->width; ++x) {
                std::uint32_t px;
                if (image->byte_order == LSBFirst)
                    px = (
                        static_cast<std::uint32_t>(srcRow[1]) << 8
                        | srcRow[0]);
                else
                    px = (
                        static_cast<std::uint32_t>(srcRow[0]) << 8
                        | srcRow[1]);
                srcRow += 2;

                const auto r = img::expandTo8Bit(
                    (px & image->red_mask) >> rShift, rBits);
                const auto g = img::expandTo8Bit(
                    (px & image->green_mask) >> gShift, gBits);
                const auto b = img::expandTo8Bit(
                    (px & image->blue_mask) >> bShift, bBits);

                dstRow[x] = img::rgbToGray(r, g, b);
            }
        }
    }
}


}
}
