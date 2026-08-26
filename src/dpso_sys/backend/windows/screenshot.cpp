#include "backend/windows/screenshot.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dpso_utils/error_get.h"
#include "dpso_utils/geometry.h"
#include "dpso_utils/windows/error.h"
#include "dpso_utils/windows/gdi.h"

#include "backend/screenshot_error.h"


namespace dpso::backend::windows {


using namespace dpso::windows;


img::ImgUPtr takeScreenshot(const Rect& rect)
{
    const Rect virtualScreenRect{
        GetSystemMetrics(SM_XVIRTUALSCREEN),
        GetSystemMetrics(SM_YVIRTUALSCREEN),
        GetSystemMetrics(SM_CXVIRTUALSCREEN),
        GetSystemMetrics(SM_CYVIRTUALSCREEN)};

    const auto captureRect = getIntersection(rect, virtualScreenRect);
    if (isEmpty(captureRect))
        throw ScreenshotError{"Rect is outside screen bounds"};

    auto screenDc = windows::getDc(nullptr);
    if (!screenDc)
        throw ScreenshotError{"GetDC(nullptr) failed"};

    auto imageDc = windows::createCompatibleDc(screenDc.get());
    if (!imageDc)
        throw ScreenshotError(
            "CreateCompatibleDC() for screen DC failed");

    windows::ObjectUPtr<HBITMAP> imageBitmap{
        CreateCompatibleBitmap(
            screenDc.get(), captureRect.w, captureRect.h)};
    if (!imageBitmap)
        throw ScreenshotError{
            "CreateCompatibleBitmap() for screen DC failed"};

    // The GetDIBits() docs say that the bitmap must not be selected
    // into a DC when calling the function.
    {
        const windows::ObjectSelector bitmapSelector{
            imageDc.get(), imageBitmap.get()};

        if (!BitBlt(
                imageDc.get(), 0, 0, captureRect.w, captureRect.h,
                screenDc.get(), captureRect.x, captureRect.y,
                SRCCOPY))
            throw ScreenshotError{
                "BitBlt(): "
                + windows::getErrorMessage(GetLastError())};
    }

    BITMAPINFOHEADER bi{};
    bi.biSize = sizeof(bi);
    bi.biWidth = captureRect.w;
    bi.biHeight = -captureRect.h;  // Invert for top-down row order.
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    img::ImgUPtr result{
        dpsoImgCreate(
            DpsoPxFormatBgra,
            captureRect.w,
            captureRect.h,
            GDI_DIBWIDTHBYTES(bi))};
    if (!result)
        throw ScreenshotError{
            std::string{"dpsoImgCreate(): "} + dpsoGetError()};

    if (!GetDIBits(
            imageDc.get(), imageBitmap.get(),
            0, captureRect.h,
            dpsoImgGetData(result.get()),
            reinterpret_cast<BITMAPINFO*>(&bi),
            DIB_RGB_COLORS))
        throw ScreenshotError{
            "GetDIBits(): "
            + windows::getErrorMessage(GetLastError())};

    return result;
}


}
