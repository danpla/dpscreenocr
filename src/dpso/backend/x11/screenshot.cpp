
#include "backend/x11/screenshot.h"

#include <cassert>

#include <X11/Xutil.h>

#include "dpso_utils/str.h"

#include "backend/screenshot_error.h"
#include "geometry.h"
#include "img.h"


// We support the following XImage depths:
//   * 32 (ARGB 8-8-8-8)
//   * 30 (XRGB 2-10-10-10)
//   * 24 (XRGB 8-8-8-8)
//   * 16 (RGB 5-6-5)
//   * 15 (RGB 5-5-5)
//
// My system also supports 1, 4, and 8, but with 8 most apps don't
// display colors correctly, so it's safe to assume that such depths
// are not common nowadays.
//
// We don't use SHM, at least for now. It requires the fixed image
// size, so it's necessary to read the whole screen to get an area
// of arbitrary size. Of course SHM is faster than XGetImage()
// when reading the whole screen (2 times faster on my machine with
// 1600x900 monitor: 10 vs 20 ms), but XGetImage() is faster for
// capturing small areas, which is more common in our case.


namespace dpso::backend::x11 {
namespace {


// The pixel type implied by XImage::*_mask and XGetPixel().
using XPixel = unsigned long;


class Screenshot : public backend::Screenshot {
public:
    explicit Screenshot(XImage* image);
    ~Screenshot();

    int getWidth() const override;
    int getHeight() const override;

    void getGrayscaleData(
        std::uint8_t* buf, int pitch) const override;
private:
    XImage* image;
};


}


Screenshot::Screenshot(XImage* image)
    : image{image}
{
    assert(image);
    assert(image->width > 0);
    assert(image->height > 0);
}


Screenshot::~Screenshot()
{
    XDestroyImage(image);
}


int Screenshot::getWidth() const
{
    return image->width;
}


int Screenshot::getHeight() const
{
    return image->height;
}


template<int bytesPerPixel>
static inline XPixel extractPixel(
    const std::uint8_t* data, int byteOrder)
{
    XPixel px{};

    if (byteOrder == LSBFirst)
        for (int i = 0; i < bytesPerPixel; ++i)
            px |= static_cast<XPixel>(data[i]) << (i * 8);
    else
        for (int i = 0; i < bytesPerPixel; ++i)
            px = (px << 8) | data[i];

    return px;
}


template<
    int bytesPerPixel,
    typename RTransformer,
    typename GTransformer,
    typename BTransformer>
static void getGrayscaleDataImpl(
    const XImage& image,
    std::uint8_t* buf,
    int pitch,
    RTransformer rTransformer,
    GTransformer gTransformer,
    BTransformer bTransformer)
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
            const auto px = extractPixel<bytesPerPixel>(
                srcRow, image.byte_order);
            srcRow += bytesPerPixel;

            dstRow[x] = img::rgbToGray(
                rTransformer((px & image.red_mask) >> rShift),
                gTransformer((px & image.green_mask) >> gShift),
                bTransformer((px & image.blue_mask) >> bShift));
        }
    }
}


template<int bytesPerPixel, typename CCTransformer>
static void getGrayscaleDataImpl(
    const XImage& image,
    std::uint8_t* buf,
    int pitch,
    CCTransformer ccTransformer)
{
    getGrayscaleDataImpl<bytesPerPixel>(
        image,
        buf,
        pitch,
        ccTransformer,
        ccTransformer,
        ccTransformer);
}


void Screenshot::getGrayscaleData(
    std::uint8_t* buf, int pitch) const
{
    if (image->bits_per_pixel == 32) {
        getGrayscaleDataImpl<4>(
                *image, buf, pitch,
                image->depth == 30
                    // XRGB 2-10-10-10
                    ? [](XPixel c) { return c / 4; }
                    // 32 (ARGB 8-8-8-8) and 24 (XRGB 8-8-8-8)
                    : [](XPixel c) { return c; });
    } else if (image->bits_per_pixel == 16) {
        const auto makeExpander = [](unsigned numBits)
        {
            return [=](XPixel c)
                {
                    return img::expandTo8Bit(c, numBits);
                };
        };

        getGrayscaleDataImpl<2>(
            *image, buf, pitch,
            makeExpander(5),
            makeExpander(image->depth == 16 ? 6 : 5),
            makeExpander(5));
    } else
        throw ScreenshotError(str::format(
            "Bit depth {} is not supported", image->bits_per_pixel));
}


std::unique_ptr<backend::Screenshot> takeScreenshot(
    Display* display, const Rect& rect)
{
    auto* screen = XDefaultScreenOfDisplay(display);
    const Rect screenRect{
        0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen)};
    const auto captureRect = getIntersection(rect, screenRect);
    if (isEmpty(captureRect))
        throw ScreenshotError("Rect is outside screen bounds");

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
        throw ScreenshotError("XGetImage() failed");

    return std::make_unique<Screenshot>(image);
}


}
