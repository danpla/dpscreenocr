
#include "backend/windows/windows_screenshot.h"

#include <cassert>
#include <utility>

#include <windows.h>

#include "backend/windows/utils.h"
#include "img.h"


namespace dpso {
namespace backend {


std::unique_ptr<WindowsScreenshot> WindowsScreenshot::take(
    const Rect& rect)
{
    const Rect virtualScreenRect{
        GetSystemMetrics(SM_XVIRTUALSCREEN),
        GetSystemMetrics(SM_YVIRTUALSCREEN),
        GetSystemMetrics(SM_CXVIRTUALSCREEN),
        GetSystemMetrics(SM_CYVIRTUALSCREEN)
    };

    const auto captureRect = getIntersection(rect, virtualScreenRect);
    if (isEmpty(captureRect))
        return nullptr;

    auto screenDc = windows::getDc(nullptr);
    if (!screenDc)
        return nullptr;

    auto imageDc = windows::createCompatibleDc(screenDc.get());
    if (!imageDc)
        return nullptr;

    windows::ObjectPtr<HBITMAP> imageBitmap(CreateCompatibleBitmap(
        screenDc.get(), captureRect.w, captureRect.h));
    if (!imageBitmap)
        return nullptr;

    // The GetDIBits() docs say that the bitmap must not be selected
    // into a DC when calling the function.
    {
        const windows::ObjectSelector bitmapSelector(
            imageDc.get(), imageBitmap.get());

        BitBlt(
            imageDc.get(), 0, 0, captureRect.w, captureRect.h,
            screenDc.get(), captureRect.x, captureRect.y,
            SRCCOPY);
    }

    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = captureRect.w;
    bi.biHeight = -captureRect.h;  // Invert for top-down rows order.
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // Pitch is called "stride" in Microsoft docs.
    const auto pitch = (captureRect.w * bi.biBitCount + 31) / 32 * 4;
    BufPtr buf(new std::uint8_t[pitch * captureRect.h]);

    GetDIBits(
        imageDc.get(), imageBitmap.get(),
        0, captureRect.h,
        buf.get(),
        reinterpret_cast<BITMAPINFO*>(&bi),
        DIB_RGB_COLORS);

    return std::unique_ptr<WindowsScreenshot>(new WindowsScreenshot(
        std::move(buf), captureRect.w, captureRect.h, pitch));
}


WindowsScreenshot::WindowsScreenshot(
        BufPtr buf, int w, int h, int pitch)
    : buf{std::move(buf)}
    , w{w}
    , h{h}
    , pitch{pitch}
{
    assert(this->buf);
    assert(w > 0);
    assert(h > 0);
    assert(pitch >= w);
}


int WindowsScreenshot::getWidth() const
{
    return w;
}


int WindowsScreenshot::getHeight() const
{
    return h;
}


void WindowsScreenshot::getGrayscaleData(
    std::uint8_t* buf, int pitch) const
{
    for (int y = 0; y < h; ++y) {
        const auto* srcRow = this->buf.get() + this->pitch * y;
        auto* dstRow = buf + pitch * y;

        for (int x = 0; x < w; ++x) {
            const auto b = srcRow[0];
            const auto g = srcRow[1];
            const auto r = srcRow[2];

            dstRow[x] = img::rgbToGray(r, g, b);
            srcRow += 4;
        }
    }
}


}
}
