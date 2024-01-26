
#include "backend/x11/x11_selection.h"

#include <charconv>
#include <cstring>

#include <X11/Xutil.h>
#include <X11/extensions/shape.h>


// We use XQueryPointer() instead of XGrabPointer(), because the
// latter fails if the pointer is already grabbed (usually by apps
// that work in fullscreen mode). Stealing focus with XSetInputFocus()
// is also not a good idea, as in this case most fullscreen apps will
// either switch to windowed mode or minimize their windows.


namespace dpso::backend {


static Point getMousePos(Display* display)
{
    Window rootWindow, childWindow;
    int rootX, rootY;
    int winX, winY;
    unsigned mask;

    XQueryPointer(
        display,
        XDefaultRootWindow(display),
        &rootWindow,
        &childWindow,
        &rootX,
        &rootY,
        &winX,
        &winY,
        &mask);

    return {rootX, rootY};
}


const auto baseDpi = 96;


static int getDpi(Display* display)
{
    // X resources are attached to the root window when the program
    // starts, so XGetDefault() will always return the same value. An
    // alternative that allows to be notified of DPI changes is to use
    // the XSettings' "Xft/DPI" property instead (on modern systems,
    // it seems to be kept in sync with "Xft.dpi").
    //
    // In addition to font scale, some modern desktop environments
    // also allow to set windows scale factor. We should probably take
    // this into account in the future. See:
    // https://wiki.archlinux.org/title/HiDPI
    const auto* dpiStr = XGetDefault(display, "Xft", "dpi");
    if (!dpiStr)
        // Xft.dpi may be unset when the default 96 is used.
        return baseDpi;

    // Xft.dpi is usually written as an integer, but Xft parses it as
    // a double. We treat it as int, ignoring the fractional part.
    if (int dpi{}; std::from_chars(
            dpiStr,
            dpiStr + std::strlen(dpiStr),
            dpi,
            10).ec == std::errc{})
        return dpi;

    return baseDpi;
}


X11Selection::X11Selection(Display* display)
    : display{display}
{
    XSetWindowAttributes windowAttrs;
    windowAttrs.override_redirect = True;

    window = XCreateWindow(
        display, XDefaultRootWindow(display),
        0, 0,
        1, 1,
        0,
        CopyFromParent,
        CopyFromParent,
        CopyFromParent,
        CWOverrideRedirect,
        &windowAttrs);
    XSelectInput(display, window, ExposureMask);

    updateBorderWidth();

    XGCValues gcval;
    gcval.foreground = XWhitePixel(display, 0);
    gcval.background = XBlackPixel(display, 0);
    gcval.line_width = borderWidth;
    gcval.line_style = LineDoubleDash;
    gcval.dashes = borderWidth * Selection::dashLen;

    gc = XCreateGC(
        display,
        window,
        GCForeground | GCBackground | GCLineWidth | GCLineStyle
            | GCDashList,
        &gcval);

    updateWindowGeometry();
    updateWindowShape();
}


X11Selection::~X11Selection()
{
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
}


bool X11Selection::getIsEnabled() const
{
    return isEnabled;
}


void X11Selection::setBorderWidth(int newBorderWidth)
{
    if (newBorderWidth == baseBorderWidth)
        return;

    baseBorderWidth = newBorderWidth;
    updateBorderWidth();

    XGCValues gcval;
    gcval.line_width = borderWidth;
    gcval.dashes = borderWidth * Selection::dashLen;

    XChangeGC(display, gc, GCLineWidth | GCDashList, &gcval);

    updateWindowGeometry();
    updateWindowShape();
}


void X11Selection::setIsEnabled(bool newIsEnabled)
{
    if (newIsEnabled == isEnabled)
        return;

    isEnabled = newIsEnabled;
    if (isEnabled) {
        origin = getMousePos(display);
        setGeometry({origin, {}});
        // We raise the window as a workaround for qtile, which as of
        // version 0.21.0 honors neither override_redirect nor
        // _NET_WM_STATE_ABOVE.
        XMapRaised(display, window);
    } else
        XUnmapWindow(display, window);

    XFlush(display);
}


Rect X11Selection::getGeometry() const
{
    return geom;
}


void X11Selection::updateStart()
{
    if (!isEnabled)
        return;

    auto newGeom = Rect::betweenPoints(origin, getMousePos(display));

    // The maximum cursor position is 1 pixel less than the size
    // of the display.
    ++newGeom.w;
    ++newGeom.h;

    setGeometry(newGeom);
}


void X11Selection::handleEvent(const XEvent& event)
{
    if (event.type == Expose
            && event.xexpose.window == window
            && event.xexpose.count == 0)
        draw();
}


void X11Selection::updateBorderWidth()
{
    borderWidth = static_cast<float>(baseBorderWidth)
        * getDpi(display) / baseDpi + 0.5f;
    if (borderWidth < 1)
        borderWidth = 1;
}


void X11Selection::updateWindowGeometry()
{
    XMoveResizeWindow(
        display,
        window,
        geom.x - borderWidth,
        geom.y - borderWidth,
        geom.w + borderWidth * 2,
        geom.h + borderWidth * 2);
}


void X11Selection::updateWindowShape()
{
    const auto sideH = geom.h;
    const auto windowW = geom.w + borderWidth * 2;
    const auto windowH = geom.h + borderWidth * 2;

    XRectangle boundingRects[4] = {
        {
            0,
            0,
            static_cast<unsigned short>(windowW),
            static_cast<unsigned short>(borderWidth)},
        {
            0,
            static_cast<short>(borderWidth),
            static_cast<unsigned short>(borderWidth),
            static_cast<unsigned short>(sideH)},
        {
            static_cast<short>(windowW - borderWidth),
            static_cast<short>(borderWidth),
            static_cast<unsigned short>(borderWidth),
            static_cast<unsigned short>(sideH)},
        {
            0,
            static_cast<short>(windowH - borderWidth),
            static_cast<unsigned short>(windowW),
            static_cast<unsigned short>(borderWidth)},
    };

    XShapeCombineRectangles(
        display,
        window,
        ShapeBounding,
        0,
        0,
        boundingRects,
        4,
        ShapeSet,
        Unsorted);

    XRectangle windowRect{
        0,
        0,
        static_cast<unsigned short>(windowW),
        static_cast<unsigned short>(windowH)};

    XShapeCombineRectangles(
        display,
        window,
        ShapeInput,
        0,
        0,
        &windowRect,
        1,
        ShapeSubtract,
        Unsorted);
}


void X11Selection::setGeometry(const Rect& newGeom)
{
    const auto newSize = getSize(newGeom) != getSize(geom);
    if (!newSize && getPos(newGeom) == getPos(geom))
        return;

    geom = newGeom;

    updateWindowGeometry();

    if (newSize)
        updateWindowShape();
}


void X11Selection::draw()
{
    XDrawRectangle(
        display,
        window,
        gc,
        borderWidth / 2,
        borderWidth / 2,
        geom.w + borderWidth,
        geom.h + borderWidth);
}


}
