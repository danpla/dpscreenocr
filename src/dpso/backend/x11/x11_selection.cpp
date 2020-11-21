
#include "backend/x11/x11_selection.h"

#include <X11/Xutil.h>
#include <X11/extensions/shape.h>


// We use XQueryPointer() instead of XGrabPointer(), because the
// latter fails if it's already grabbed (usually by apps that work
// in fullscreen mode). Stealing focus with XSetInputFocus() is also
// not a good idea: while some fullscreen apps switch to windowed
// mode, others minimize their windows.


namespace dpso {
namespace backend {


static Point getMousePosition(Display* display)
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


static Rect getCurrentSelectionRect(
    Display* display, const Point& origin)
{
    auto result = Rect::betweenPoints(
        origin, getMousePosition(display));

    // The maximum cursor position is 1 pixel less than the size
    // of the display.
    ++result.w;
    ++result.h;

    return result;
}


const int borderWidth = 4;


X11Selection::X11Selection(Display* display)
    : display{display}
    , window{}
    , gc{}
    , isEnabled{}
    , origin{}
    , geom{}
{
    XSetWindowAttributes windowAttrs;
    windowAttrs.override_redirect = True;

    window = XCreateWindow(
        display, XDefaultRootWindow(display),
        geom.x - borderWidth, geom.y - borderWidth,
        geom.w + borderWidth * 2, geom.h + borderWidth * 2,
        0,
        CopyFromParent,
        CopyFromParent,
        CopyFromParent,
        CWOverrideRedirect,
        &windowAttrs);
    XSelectInput(display, window, ExposureMask);

    XGCValues gcval;
    gcval.foreground = XWhitePixel(display, 0);
    gcval.background = XBlackPixel(display, 0);
    gcval.line_width = borderWidth;
    gcval.line_style = LineDoubleDash;
    gcval.dashes = borderWidth * 3;

    gc = XCreateGC(
        display,
        window,
        GCForeground | GCBackground | GCLineWidth | GCLineStyle
            | GCDashList,
        &gcval);

    reshapeWindow();
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


void X11Selection::setIsEnabled(bool newIsEnabled)
{
    if (newIsEnabled == isEnabled)
        return;

    isEnabled = newIsEnabled;
    if (isEnabled) {
        origin = getMousePosition(display);
        setGeometry({origin.x, origin.y, 0, 0});
        XMapWindow(display, window);
    } else
        XUnmapWindow(display, window);

    XFlush(display);
}


Rect X11Selection::getGeometry() const
{
    return geom;
}


void X11Selection::setGeometry(const Rect& newGeom)
{
    XWindowChanges windowChanges;
    unsigned valueMask = 0;

    if (newGeom.x != geom.x) {
        windowChanges.x = newGeom.x - borderWidth;
        valueMask |= CWX;
        geom.x = newGeom.x;
    }

    if (newGeom.y != geom.y) {
        windowChanges.y = newGeom.y - borderWidth;
        valueMask |= CWY;
        geom.y = newGeom.y;
    }

    if (newGeom.w != geom.w) {
        windowChanges.width = newGeom.w + borderWidth * 2;
        valueMask |= CWWidth;
        geom.w = newGeom.w;
    }

    if (newGeom.h != geom.h) {
        windowChanges.height = newGeom.h + borderWidth * 2;
        valueMask |= CWHeight;
        geom.h = newGeom.h;
    }

    if (valueMask == 0)
        return;

    XConfigureWindow(display, window, valueMask, &windowChanges);
    if (valueMask & (CWWidth | CWHeight))
        reshapeWindow();
}


void X11Selection::updateStart()
{
    if (!isEnabled)
        return;

    setGeometry(getCurrentSelectionRect(display, origin));
}


void X11Selection::handleEvent(const XEvent& event)
{
    if (!isEnabled
            || event.type != Expose
            || event.xexpose.window != window
            || event.xexpose.count > 0)
        return;

    drawSelection();
}


void X11Selection::reshapeWindow()
{
    const auto sideH = geom.h;
    const auto windowW = geom.w + borderWidth * 2;
    const auto windowH = geom.h + borderWidth * 2;

    XRectangle boundingRects[4] = {
        {
            0,
            0,
            static_cast<unsigned short>(windowW),
            static_cast<unsigned short>(borderWidth)
        },
        {
            0,
            static_cast<short>(borderWidth),
            static_cast<unsigned short>(borderWidth),
            static_cast<unsigned short>(sideH)
        },
        {
            static_cast<short>(windowW - borderWidth),
            static_cast<short>(borderWidth),
            static_cast<unsigned short>(borderWidth),
            static_cast<unsigned short>(sideH)
        },
        {
            0,
            static_cast<short>(windowH - borderWidth),
            static_cast<unsigned short>(windowW),
            static_cast<unsigned short>(borderWidth)
        },
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

    XRectangle windowRect = {
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


void X11Selection::drawSelection()
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
}
