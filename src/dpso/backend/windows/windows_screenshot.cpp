
#include "backend/windows/windows_screenshot.h"

#include <cassert>

#include <windows.h>

#include "img.h"


namespace dpso {
namespace backend {


WindowsScreenshot* WindowsScreenshot::take(const Rect& rect)
{
    const Rect virtualScreenRect {
        GetSystemMetrics(SM_XVIRTUALSCREEN),
        GetSystemMetrics(SM_YVIRTUALSCREEN),
        GetSystemMetrics(SM_CXVIRTUALSCREEN),
        GetSystemMetrics(SM_CYVIRTUALSCREEN)
    };

    const auto captureRect = rect.getIntersection(virtualScreenRect);
    if (captureRect.empty())
        return nullptr;

    auto screenDc = GetDC(nullptr);
    if (!screenDc)
        return nullptr;

    auto imageDc = CreateCompatibleDC(screenDc);
    if (!imageDc) {
        ReleaseDC(nullptr, screenDc);
        return nullptr;
    }

    auto imageBitmap = CreateCompatibleBitmap(
        screenDc, captureRect.w, captureRect.h);
    if (!imageBitmap) {
        DeleteDC(imageDc);
        ReleaseDC(nullptr, screenDc);
        return nullptr;
    }

    auto oldBitmap = SelectObject(imageDc, imageBitmap);
    BitBlt(
        imageDc, 0, 0, captureRect.w, captureRect.h,
        screenDc, captureRect.x, captureRect.y,
        SRCCOPY);
    // The GetDIBits() docs say that the bitmap must not be selected
    // into a DC when calling the function.
    SelectObject(imageDc, oldBitmap);

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
    auto* buf = new std::uint8_t[pitch * captureRect.h];

    GetDIBits(
        imageDc, imageBitmap,
        0, captureRect.h,
        buf,
        reinterpret_cast<BITMAPINFO*>(&bi),
        DIB_RGB_COLORS);

    DeleteObject(imageBitmap);
    DeleteDC(imageDc);
    ReleaseDC(nullptr, screenDc);

    return new WindowsScreenshot(
        buf, captureRect.w, captureRect.h, pitch);
}


WindowsScreenshot::WindowsScreenshot(
        unsigned char* buf, int w, int h, int pitch)
    : buf {buf}
    , w {w}
    , h {h}
    , pitch {pitch}
{
    assert(buf);
    assert(w > 0);
    assert(h > 0);
    assert(pitch >= w);
}


WindowsScreenshot::~WindowsScreenshot()
{
    delete[] buf;
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
        const auto* srcRow = this->buf + this->pitch * y;
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
