
#include "backend/windows/windows_selection.h"

#include "backend/backend_error.h"


// We use window regions (SetWindowRgn()) for selection.
//
// Another choice was a layered window (WS_EX_LAYERED) with
// LWA_COLORKEY. However, this approach has several issues that happen
// during resizing, especially when Aero is disabled:
//
//   * The bigger the window, the higher is CPU load.
//
//   * Resizing a layered window forces all underlying windows to be
//     repainted, even under transparent areas. This results in
//     flashing widgets with some applications.
//
//     This is also probably one of the reasons of high CPU load.
//
//   * Layered window always repaints its background, even with dummy
//     WM_ERASEBKGND that returns 1; if hbrBackground is NULL, the
//     color is black. During resizing, the background flashes before
//     WM_PAINT event. Setting the background color to LWA_COLORKEY
//     results in blinking.
//
//     Moving all drawing from WM_PAINT to WM_ERASEBKGND doesn't solve
//     the issue completely: when the window is enlarged, its right
//     and bottom portions flash with background color even before
//     WM_ERASEBKGND event.
//
//     In combination with all previous issues, this also causes
//     vertical stuttering.
//
// Window regions have no such problems; they work well both with
// and without Aero.
//
// Another good thing is that Windows don't prevent click-trough
// behavior during resizing, at the moments when cursor is on the
// visible part of the selection; that was not possible on X11 without
// making the whole window "transparent" for the mouse. In other
// words, we don't need WS_EX_TRANSPARENT, which only works in
// combination with WS_EX_LAYERED.


namespace dpso {
namespace backend {


const int borderWidth96Dpi = 4;


// TODO: Support dynamic DPI changes (WM_DPICHANGED) on Windows 8.1
// and newer.
static int getBorderWidth()
{
    auto dc = windows::getDc(nullptr);
    if (!dc)
        return borderWidth96Dpi;

    // LOGPIXELSX and LOGPIXELSY are identical (as well as values
    // returned by GetDpiForMonitor()).
    const auto dpi = GetDeviceCaps(dc.get(), LOGPIXELSX);

    const auto scale = dpi / 96.0f;

    int scaledBorderWidth = borderWidth96Dpi * scale + 0.5f;
    // The width must be even since we will draw it with pens; the
    // middle of the pen is placed on the edge of a shape. See draw().
    if (scaledBorderWidth % 2)
        ++scaledBorderWidth;

    return scaledBorderWidth;
}


static Point getMousePosition()
{
    POINT point;
    GetCursorPos(&point);
    return {point.x, point.y};
}


static void throwLastError(const char* description)
{
    throw BackendError(
        std::string(description) + ": "
        + windows::getLastErrorMessage());
}


const auto* const windowClassName = "DpsoSelectionWindow";


static void registerWindowClass(HINSTANCE instance, WNDPROC wndProc)
{
    static bool registered;
    if (registered)
        return;

    WNDCLASSEXA wcx;
    wcx.cbSize = sizeof(WNDCLASSEX);
    wcx.style = CS_VREDRAW | CS_HREDRAW;
    wcx.lpfnWndProc = wndProc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = instance;
    wcx.hIcon = nullptr;
    wcx.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcx.hbrBackground = nullptr;
    wcx.lpszMenuName = nullptr;
    wcx.lpszClassName = windowClassName;
    wcx.hIconSm = nullptr;

    RegisterClassExA(&wcx);
    registered = true;
}


WindowsSelection::WindowsSelection(HINSTANCE instance)
    : borderWidth{getBorderWidth()}
    , isEnabled{}
    , origin{}
    , geom{}
    , window{}
    , dashPenPattern{}
    , pens{}
{
    registerWindowClass(instance, WindowsSelection::wndProc);
    window.reset(CreateWindowExA(
        WS_EX_TOPMOST
            | WS_EX_TOOLWINDOW, // Hide from taskbar.
        windowClassName,
        "Selection",
        WS_POPUP,
        geom.x - borderWidth, geom.y - borderWidth,
        geom.w + borderWidth * 2,
        geom.h + borderWidth * 2,
        nullptr,
        nullptr,
        instance,
        nullptr));

    if (!window)
        throwLastError("Can't create selection window");

    if (!SetPropA(window.get(), "this", this))
        throwLastError("Can't set window property");

    updateWindowRegion();

    createPens();
}


void WindowsSelection::createPens()
{
    const auto commonStyle = (
        PS_GEOMETRIC | PS_ENDCAP_FLAT | PS_JOIN_MITER);

    // White background.
    LOGBRUSH lb{BS_SOLID, RGB(255, 255, 255), 0};

    pens[0].reset(ExtCreatePen(
        commonStyle | PS_SOLID, borderWidth, &lb, 0, nullptr));
    if (!pens[0])
        throwLastError("Can't create background pen");

    // Black dashes.
    lb.lbColor = RGB(0, 0, 0);

    // The docs don't say whether ExtCreatePen() copies the array, so
    // it's a class member just in case.
    dashPenPattern[0] = borderWidth * 3;
    dashPenPattern[1] = borderWidth * 3;

    pens[1].reset(ExtCreatePen(
        commonStyle | PS_USERSTYLE, borderWidth, &lb,
        sizeof(dashPenPattern) / sizeof(*dashPenPattern),
        dashPenPattern));
    if (!pens[1])
        throwLastError("Can't create dash pen");
}


bool WindowsSelection::getIsEnabled() const
{
    return isEnabled;
}


void WindowsSelection::setIsEnabled(bool newIsEnabled)
{
    if (newIsEnabled == isEnabled)
        return;

    isEnabled = newIsEnabled;

    if (isEnabled) {
        origin = getMousePosition();
        setGeometry({origin.x, origin.y, 0, 0});
    }

    ShowWindow(window.get(), isEnabled ? SW_SHOWNA : SW_HIDE);
}


Rect WindowsSelection::getGeometry() const
{
    return geom;
}


void WindowsSelection::update()
{
    if (!isEnabled)
        return;

    auto newGeom = Rect::betweenPoints(origin, getMousePosition());
    // The maximum cursor position is 1 pixel less than the size of
    // the display.
    ++newGeom.w;
    ++newGeom.h;

    setGeometry(newGeom);
}


LRESULT CALLBACK WindowsSelection::wndProc(
    HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* windowsSelection = static_cast<WindowsSelection*>(
        GetPropA(wnd, "this"));
    if (!windowsSelection)
        // The window is just created; we don't reach SetProp() call
        // yet.
        return DefWindowProc(wnd, msg, wParam, lParam);

    return windowsSelection->processMessage(msg, wParam, lParam);
}


LRESULT WindowsSelection::processMessage(
    UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_ERASEBKGND) {
        return 1;
    } else if (msg == WM_PAINT) {
        static PAINTSTRUCT ps;
        auto dc = BeginPaint(window.get(), &ps);
        draw(dc);
        EndPaint(window.get(), &ps);
        return 0;
    } else
        return DefWindowProc(window.get(), msg, wParam, lParam);
}


void WindowsSelection::setGeometry(const Rect& newGeom)
{
    UINT flags = 0;

    if (newGeom.x == geom.x && newGeom.y == geom.y)
        flags |= SWP_NOMOVE;
    if (newGeom.w == geom.w && newGeom.h == geom.h)
        flags |= SWP_NOSIZE;

    if (flags == (SWP_NOMOVE | SWP_NOSIZE))
        return;

    flags |= SWP_NOZORDER | SWP_NOACTIVATE;
    geom = newGeom;

    SetWindowPos(
        window.get(), 0,
        geom.x - borderWidth, geom.y - borderWidth,
        geom.w + borderWidth * 2, geom.h + borderWidth * 2,
        flags);

    if (flags & SWP_NOSIZE)
        return;

    updateWindowRegion();
}


void WindowsSelection::draw(HDC dc)
{
    const auto rectLeft = borderWidth / 2;
    const auto rectTop = borderWidth / 2;

    // We need to add an extra pixel to right and bottom because of
    // how Rectangle() places the border at these coordinates.
    const auto rectRight = rectLeft + geom.w + borderWidth + 1;
    const auto rectBottom = rectTop + geom.h + borderWidth + 1;

    const windows::ObjectSelector brushSelector(
        dc, GetStockObject(NULL_BRUSH));
    for (const auto& pen : pens) {
        const windows::ObjectSelector penSelector(dc, pen.get());
        Rectangle(dc, rectLeft, rectTop, rectRight, rectBottom);
    }
}


void WindowsSelection::updateWindowRegion()
{
    windows::ObjectPtr<HRGN> region(CreateRectRgn(
        0, 0, geom.w + borderWidth * 2, geom.h + borderWidth * 2));
    if (!region)
        return;

    windows::ObjectPtr<HRGN> holeRegion(CreateRectRgn(
        borderWidth, borderWidth,
        borderWidth + geom.w, borderWidth + geom.h));
    if (!holeRegion)
        return;

    CombineRgn(region.get(), region.get(), holeRegion.get(), RGN_XOR);
    if (SetWindowRgn(window.get(), region.get(), TRUE) != 0)
        // The region is now owned by the window.
        region.release();
}


}
}
