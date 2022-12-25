
#include "backend/windows/windows_screenshot.h"

#include <cassert>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "backend/screenshot_error.h"
#include "backend/windows/utils/error.h"
#include "backend/windows/utils/gdi.h"
#include "geometry.h"
#include "img.h"


namespace dpso::backend {
namespace {


using DataUPtr = std::unique_ptr<std::uint8_t[]>;


class WindowsScreenshot : public Screenshot {
public:
    WindowsScreenshot(DataUPtr data, int w, int h, int pitch);

    int getWidth() const override;
    int getHeight() const override;

    void getGrayscaleData(
        std::uint8_t* buf, int pitch) const override;
private:
    DataUPtr data;
    int w;
    int h;
    int pitch;
};


}


WindowsScreenshot::WindowsScreenshot(
        DataUPtr data, int w, int h, int pitch)
    : data{std::move(data)}
    , w{w}
    , h{h}
    , pitch{pitch}
{
    assert(this->data);
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
        const auto* srcRow = data.get() + this->pitch * y;
        auto* dstRow = buf + pitch * y;

        for (int x = 0; x < w; ++x) {
            dstRow[x] = img::rgbToGray(
                srcRow[2], srcRow[1], srcRow[0]);
            srcRow += 4;
        }
    }
}


std::unique_ptr<Screenshot> takeWindowsScreenshot(const Rect& rect)
{
    const Rect virtualScreenRect{
        GetSystemMetrics(SM_XVIRTUALSCREEN),
        GetSystemMetrics(SM_YVIRTUALSCREEN),
        GetSystemMetrics(SM_CXVIRTUALSCREEN),
        GetSystemMetrics(SM_CYVIRTUALSCREEN)
    };

    const auto captureRect = getIntersection(rect, virtualScreenRect);
    if (isEmpty(captureRect))
        throw ScreenshotError("Rect is outside screen bounds");

    auto screenDc = windows::getDc(nullptr);
    if (!screenDc)
        throw ScreenshotError("GetDC(nullptr) failed");

    auto imageDc = windows::createCompatibleDc(screenDc.get());
    if (!imageDc)
        throw ScreenshotError(
            "CreateCompatibleDC() for screen DC failed");

    windows::ObjectUPtr<HBITMAP> imageBitmap(CreateCompatibleBitmap(
        screenDc.get(), captureRect.w, captureRect.h));
    if (!imageBitmap)
        throw ScreenshotError(
            "CreateCompatibleBitmap() for screen DC failed");

    // The GetDIBits() docs say that the bitmap must not be selected
    // into a DC when calling the function.
    {
        const windows::ObjectSelector bitmapSelector(
            imageDc.get(), imageBitmap.get());

        if (!BitBlt(
                imageDc.get(), 0, 0, captureRect.w, captureRect.h,
                screenDc.get(), captureRect.x, captureRect.y,
                SRCCOPY))
            throw ScreenshotError(
                "BitBlt(): "
                + windows::getErrorMessage(GetLastError()));
    }

    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(bi);
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
    auto data = std::make_unique<std::uint8_t[]>(
        pitch * captureRect.h);

    if (!GetDIBits(
            imageDc.get(), imageBitmap.get(),
            0, captureRect.h,
            data.get(),
            reinterpret_cast<BITMAPINFO*>(&bi),
            DIB_RGB_COLORS))
        throw ScreenshotError(
            "GetDIBits(): "
            + windows::getErrorMessage(GetLastError()));

    return std::make_unique<WindowsScreenshot>(
        std::move(data), captureRect.w, captureRect.h, pitch);
}


}
