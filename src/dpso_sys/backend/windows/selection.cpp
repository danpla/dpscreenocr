#include "backend/windows/selection.h"

#include "dpso_utils/windows/error.h"
#include "dpso_utils/windows/module.h"

#include "backend/backend_error.h"


// Transparent window areas
// ========================
//
// We use window regions (SetWindowRgn()) for selection.
//
// Another choice was a layered window (WS_EX_LAYERED) with
// LWA_COLORKEY. However, this approach has several issues that happen
// during resizing, especially when Aero is disabled:
//
// * The bigger the window, the higher is CPU load.
//
// * Resizing a layered window forces all underlying windows to be
//   repainted, even under transparent areas. This results in flashing
//   widgets in some applications.
//
//   This is also probably one of the reasons of high CPU load.
//
// * A layered window always repaints its background, even with dummy
//   WM_ERASEBKGND that returns 1; if hbrBackground is NULL, the color
//   is black. During resizing, the background flashes before
//   WM_PAINT. Setting the background color to LWA_COLORKEY results in
//   blinking.
//
//   Moving drawing from WM_PAINT to WM_ERASEBKGND doesn't solve the
//   issue completely: when the window is enlarged, its right and
//   bottom portions flash with the background color even before
//   WM_ERASEBKGND.
//
//   In combination with all previous issues, this also causes the
//   vertical stuttering.
//
// Window regions have no such problems; they work well both with and
// without Aero.
//
// Another good thing is that Windows don't prevent the click-trough
// behavior during resizing, at the moments when cursor is on the
// visible part of the selection; that was not possible on X11 without
// making the whole window "transparent" for the mouse. In other
// words, we don't need WS_EX_TRANSPARENT, which only works in
// combination with WS_EX_LAYERED.
//
//
// Dynamic DPI changes
// ===================
//
// On Windows 8.1 and newer, the selection border dynamically scales
// according to the display DPI if a per-monitor DPI awareness is set.
// Since this is a library, we must not permanently set the process
// DPI awareness; we can only change it temporarily (or permanently
// for our own thread) with SetThreadDpiAwarenessContext() when
// running on Windows 10 version 1607 or newer. On older versions, we
// will use the DPI awareness of the process, whatever it may be.
//
// Although the Windows backend actually runs in a separate thread, we
// don't rely on this in the selection handler. Instead of setting a
// permanent per-monitor DPI awareness for the thread, we use a RAII
// wrapper around SetThreadDpiAwarenessContext() for Windows API calls
// that use DPI virtualization.


namespace dpso::backend::windows {
namespace {


dpso::windows::ModuleUPtr user32Dll{LoadLibraryW(L"user32")};
dpso::windows::ModuleUPtr shcoreDll{LoadLibraryW(L"shcore")};

DPSO_WIN_DLL_FN(
    user32Dll.get(),
    SetThreadDpiAwarenessContext,
    DPI_AWARENESS_CONTEXT (WINAPI *)(DPI_AWARENESS_CONTEXT));
DPSO_WIN_DLL_FN(
    shcoreDll.get(),
    GetDpiForMonitor,
    HRESULT (WINAPI *)(HMONITOR, int, UINT*, UINT*));


class ThreadDpiAwarenessContextGuard {
public:
    ThreadDpiAwarenessContextGuard()
        : oldDpiContext{}
    {
        // The v2 awareness was added later (in Windows 10 1703) than
        // SetThreadDpiAwarenessContext() (added in Windows 10 1607),
        // so we use v1 to fill the gap. Fortunately, the selection
        // doesn't have non-client areas, so all v2 improvements are
        // irrelevant in our case.
        if (SetThreadDpiAwarenessContextFn)
            oldDpiContext = SetThreadDpiAwarenessContextFn(
                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    }

    ~ThreadDpiAwarenessContextGuard()
    {
        if (oldDpiContext)
            SetThreadDpiAwarenessContextFn(oldDpiContext);
    }
private:
    DPI_AWARENESS_CONTEXT oldDpiContext;
};


const auto baseDpi = 96;


// Get DPI of monitor containing the point.
int getDpi(const Point& point)
{
    // Windows 10 has GetDpiForWindow(), but we use GetDpiForMonitor()
    // for Windows 8.1 support.
    if (GetDpiForMonitorFn) {
        auto monitor = MonitorFromPoint(
            {point.x, point.y}, MONITOR_DEFAULTTONEAREST);

        // MDT_EFFECTIVE_DPI is an enumerator rather than a macro.
        const auto mdtFffectiveDpi = 0;

        UINT xDpi, yDpi;
        if (SUCCEEDED(GetDpiForMonitorFn(
                monitor, mdtFffectiveDpi, &xDpi, &yDpi)))
            return xDpi;
    }

    if (auto dc = dpso::windows::getDc(nullptr))
        return GetDeviceCaps(dc.get(), LOGPIXELSX);

    return baseDpi;
}


Point getMousePos()
{
    POINT point;
    GetCursorPos(&point);
    return {point.x, point.y};
}


[[noreturn]]
void throwLastError(const char* description)
{
    throw BackendError(
        std::string(description) + ": "
        + dpso::windows::getErrorMessage(GetLastError()));
}


const auto* const windowClassName = L"DpsoSelectionWindow";
const auto* const thisPropName = L"this";


void registerWindowClass(HINSTANCE instance, WNDPROC wndProc)
{
    static bool registered;
    if (registered)
        return;

    WNDCLASSEXW wcx;
    wcx.cbSize = sizeof(wcx);
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

    if (RegisterClassExW(&wcx) == 0)
        throwLastError("Can't register selection window class");

    registered = true;
}


}


Selection::Selection(HINSTANCE instance)
    : dpi{baseDpi}
{
    registerWindowClass(instance, Selection::wndProc);

    const ThreadDpiAwarenessContextGuard dpiAwarenessGuard;

    window.reset(CreateWindowExW(
        WS_EX_TOPMOST
            | WS_EX_TOOLWINDOW, // Hide from taskbar.
        windowClassName,
        L"Selection",
        WS_POPUP,
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        instance,
        nullptr));

    if (!window)
        throwLastError("Can't create selection window");

    if (!SetPropW(window.get(), thisPropName, this))
        throwLastError("Can't set window property");

    // WM_DPICHANGED is only sent when a window is moved to a display
    // with a different DPI, so we have to query the initial value
    // manually. We also need this when we are not in a per-monitor
    // DPI aware mode, in which case DPI will always remain the same
    // and the selection will automatically be scaled by Windows.
    //
    // Note that getDpi() is not in the initialization list since it
    // should be protected by the DPI awareness guard.
    dpi = getDpi(origin);

    updateBorderWidth();
    updatePens();
    updateWindowGeometry();
    updateWindowRegion();
}


Selection::~Selection()
{
    RemovePropW(window.get(), thisPropName);
}


bool Selection::getIsEnabled() const
{
    return isEnabled;
}


void Selection::setIsEnabled(bool newIsEnabled)
{
    if (newIsEnabled == isEnabled)
        return;

    isEnabled = newIsEnabled;

    if (isEnabled) {
        const ThreadDpiAwarenessContextGuard dpiAwarenessGuard;

        origin = getMousePos();
        setGeometry({origin, {}});
    }

    ShowWindow(window.get(), isEnabled ? SW_SHOWNA : SW_HIDE);
}


void Selection::setBorderWidth(int newBorderWidth)
{
    if (newBorderWidth == baseBorderWidth)
        return;

    const ThreadDpiAwarenessContextGuard dpiAwarenessGuard;

    baseBorderWidth = newBorderWidth;

    updateBorderWidth();
    updatePens();
    updateWindowGeometry();
    updateWindowRegion();
}


Rect Selection::getGeometry() const
{
    return geom;
}


void Selection::update()
{
    if (!isEnabled)
        return;

    const ThreadDpiAwarenessContextGuard dpiAwarenessGuard;

    auto newGeom = Rect::betweenPoints(origin, getMousePos());
    // The maximum cursor position is 1 pixel less than the size of
    // the display.
    ++newGeom.w;
    ++newGeom.h;

    setGeometry(newGeom);
}


LRESULT CALLBACK Selection::wndProc(
    HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* selection = static_cast<Selection*>(
        GetPropW(wnd, thisPropName));
    if (!selection)
        // The window is just created; we don't reach SetProp() yet.
        return DefWindowProcW(wnd, msg, wParam, lParam);

    return selection->processMessage(msg, wParam, lParam);
}


LRESULT Selection::processMessage(
    UINT msg, WPARAM wParam, LPARAM lParam)
{
    // We don't need a ThreadDpiAwarenessContextGuard guard here,
    // since the thread is automatically switched to the DPI context
    // of the window when the window procedure is called (the DPI
    // context of the window is set to the context of the calling
    // thread when the window is created).

    switch (msg) {
    case WM_DPICHANGED:
        // We intentionally ignore the suggested rect from lParam. It
        // will never be correct in our case, since Windows will keep
        // the position and linearly scale the size. As a result, the
        // origin point of our selection will be shifted and the
        // opposite point will come out from under the cursor.
        //
        // There is the WM_GETDPISCALEDSIZE event for the v2
        // per-monitor awareness that allows to set a custom rect for
        // WM_DPICHANGED, but it's only called on interactive dragging
        // and resizing, not for SetWindowPos().

        dpi = HIWORD(wParam);

        updateBorderWidth();
        updatePens();
        updateWindowGeometry();
        updateWindowRegion();

        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        auto dc = BeginPaint(window.get(), &ps);
        draw(dc);
        EndPaint(window.get(), &ps);

        return 0;
    }
    }

    return DefWindowProcW(window.get(), msg, wParam, lParam);
}


void Selection::updateBorderWidth()
{
    borderWidth = static_cast<float>(baseBorderWidth)
        * dpi / baseDpi + 0.5f;
    if (borderWidth < 1)
        borderWidth = 1;
}


void Selection::updatePens()
{
    const auto commonStyle =
        PS_GEOMETRIC | PS_ENDCAP_FLAT | PS_JOIN_MITER;

    // White background.
    LOGBRUSH lb{BS_SOLID, RGB(255, 255, 255), 0};

    pens[0].reset(ExtCreatePen(
        commonStyle | PS_SOLID, borderWidth, &lb, 0, nullptr));

    // Black dashes.
    lb.lbColor = RGB(0, 0, 0);

    const DWORD dashWidth = borderWidth * Selection::dashLen;
    const DWORD pattern[]{dashWidth, dashWidth};

    pens[1].reset(ExtCreatePen(
        commonStyle | PS_USERSTYLE, borderWidth, &lb, 2, pattern));
}


void Selection::updateWindowGeometry()
{
    SetWindowPos(
        window.get(), nullptr,
        geom.x - borderWidth, geom.y - borderWidth,
        geom.w + borderWidth * 2, geom.h + borderWidth * 2,
        SWP_NOZORDER | SWP_NOACTIVATE);
}


void Selection::updateWindowRegion()
{
    dpso::windows::ObjectUPtr<HRGN> region{CreateRectRgn(
        0, 0, geom.w + borderWidth * 2, geom.h + borderWidth * 2)};
    if (!region)
        return;

    dpso::windows::ObjectUPtr<HRGN> holeRegion{CreateRectRgn(
        borderWidth, borderWidth,
        borderWidth + geom.w, borderWidth + geom.h)};
    if (!holeRegion)
        return;

    CombineRgn(region.get(), region.get(), holeRegion.get(), RGN_XOR);
    if (SetWindowRgn(window.get(), region.get(), TRUE) != 0)
        // The region is now owned by the window.
        region.release();
}


void Selection::setGeometry(const Rect& newGeom)
{
    const auto newSize = getSize(newGeom) != getSize(geom);
    if (!newSize && getPos(newGeom) == getPos(geom))
        return;

    geom = newGeom;

    updateWindowGeometry();

    if (newSize)
        updateWindowRegion();
}


void Selection::draw(HDC dc)
{
    const auto rectLeft = borderWidth / 2;
    const auto rectTop = borderWidth / 2;

    // By GDI conventions, right and bottom edges are not part of the
    // rectangle, so we need to add an extra pixel.
    const auto rectRight = rectLeft + geom.w + borderWidth + 1;
    const auto rectBottom = rectTop + geom.h + borderWidth + 1;

    const dpso::windows::ObjectSelector brushSelector{
        dc, GetStockObject(NULL_BRUSH)};
    for (const auto& pen : pens) {
        if (!pen)
            continue;

        const dpso::windows::ObjectSelector penSelector{
            dc, pen.get()};
        Rectangle(dc, rectLeft, rectTop, rectRight, rectBottom);
    }
}


}
