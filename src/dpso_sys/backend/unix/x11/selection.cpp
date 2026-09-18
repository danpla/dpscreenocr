#include "backend/unix/x11/selection.h"

#include <algorithm>
#include <charconv>
#include <cstring>

#include <X11/Xutil.h>
#include <X11/extensions/shape.h>


// Exposure events
// ===============
//
// Since we handle X11 events in a non-blocking fashion in fixed
// update steps, there is no guarantee that invoking a routine that
// generates an event will immediately add that event to the current
// event queue; instead, the event may only appear on the next update
// step. This is usually not a problem, but if we only draw in
// response to the Expose event, a missing Expose means missing a
// repaint, which results in a jagged selection border when resizing.
//
// To fix this issue without an expensive XSync() call, we don't rely
// on Expose and instead explicitly queue a redraw whenever we know
// that we need to repaint at the end of the current update step. We
// still listen to Expose to handle expositions that originate from
// outside, such as when another override-redirect window (e.g., a
// pop-up menu) appears on top of our selection.
//
//
// Dash pattern
// ============
//
// Previously, we used XDrawRectangle() to draw the double-dash frame
// directly on the window, but eventually it turns our to be a major
// performance issue. Despite the fact that the rectangle drawn this
// way consists of strictly vertical and horizontal lines, the X.Org
// implementation makes zero effort to optimize drawing: it takes the
// corner points of the rectangle and uses a generic polygon drawing
// algorithm that makes tons of computations for every single dash in
// the line. As a result, selecting a large area with a 1-2 px dash
// length burns CPU and, when compositing and VSync are enabled, can
// easily drop the framerate down to 2-4 FPS.
//
// To fix this, we draw the dash pattern on an off-screen pixmaps,
// which is then used to tile the selection borders.
//
//
// Mouse pointer handling
// ======================
//
// We use XQueryPointer() instead of XGrabPointer(), because the
// latter fails if the pointer is already grabbed, usually by apps
// that work in fullscreen mode. Using XSetInputFocus() to steal focus
// from such apps usually forces them to switch to windowed mode or
// minimize their windows. In contrast, the XQueryPointer() approach
// does not interfere with other programs, allowing us to select areas
// even in full-screen applications.


namespace dpso::backend::x11 {
namespace {


Point getMousePos(Display* display)
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


int getDpi(Display* display)
{
    // X resources are attached to the root window when the program
    // starts, so XGetDefault() always returns the same value. An
    // alternative that notifies of DPI changes is the "Xft/DPI"
    // property of XSettings (on modern systems, it seems to be kept
    // in sync with "Xft.dpi").
    //
    // In addition to the font scale, some modern desktop environments
    // allow setting the window scale factor. We should probably take
    // this into account in the future. See:
    //
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
            dpi).ec == std::errc{})
        return dpi;

    return baseDpi;
}


}


Selection::Selection(Display* display)
    : display{display}
    , rootWindow{XDefaultRootWindow(display)}
    , screenNum{XDefaultScreen(display)}
{
    XSetWindowAttributes windowAttrs;
    windowAttrs.event_mask = ExposureMask;
    windowAttrs.override_redirect = true;

    window = WindowHandle{
        display,
        XCreateWindow(
            display, rootWindow,
            0, 0, 1, 1,
            0,
            CopyFromParent,
            InputOutput,
            CopyFromParent,
            CWEventMask | CWOverrideRedirect,
            &windowAttrs)};

    XRectangle emptyRect{};
    XShapeCombineRectangles(
        display, window,
        ShapeInput, 0, 0, &emptyRect, 1, ShapeSet, Unsorted);

    const auto createGc = [&](XGCValues& val, unsigned long valMask)
    {
        return GcHandle{
            display,
            XCreateGC(display, rootWindow, valMask, &val)};
    };

    XGCValues borderGcval;
    borderGcval.fill_style = FillTiled;

    for (auto& gc : borderGcs)
        gc = createGc(borderGcval, GCFillStyle);

    XGCValues tileGcval;
    tileGcval.foreground = XWhitePixel(display, screenNum);
    tileGcval.background = XBlackPixel(display, screenNum);
    tileGcval.line_style = LineDoubleDash;

    tileGc = createGc(
        tileGcval, GCForeground | GCBackground | GCLineStyle);

    updateBorderProperties();
    updateWindow();
}


bool Selection::getIsEnabled() const
{
    return isEnabled;
}


void Selection::setBorderWidth(int newBorderWidth)
{
    if (newBorderWidth == baseBorderWidth)
        return;

    baseBorderWidth = newBorderWidth;

    updateBorderProperties();

    if (isEnabled) {
        updateWindow();
        draw();
        XFlush(display);
    }
}


void Selection::setIsEnabled(bool newIsEnabled)
{
    if (newIsEnabled == isEnabled)
        return;

    isEnabled = newIsEnabled;

    if (isEnabled) {
        origin = getMousePos(display);
        geom = {origin, {}};

        updateWindow();

        // We raise the window as a workaround for qtile, which as of
        // version 0.21.0 honors neither override_redirect nor
        // _NET_WM_STATE_ABOVE.
        XMapRaised(display, window);

        draw();
    } else
        XUnmapWindow(display, window);

    XFlush(display);
}


Rect Selection::getGeometry() const
{
    return geom;
}


void Selection::updateStart()
{
    if (!isEnabled)
        return;

    auto newGeom = Rect::betweenPoints(origin, getMousePos(display));

    // The maximum cursor position is 1 pixel smaller than the size of
    // the display.
    ++newGeom.w;
    ++newGeom.h;

    if (newGeom == geom)
        return;

    geom = newGeom;
    updateWindow();
    needRedraw = true;
}


bool Selection::handleEvent(const XEvent& event)
{
    if (event.xany.window != window)
        return false;

    if (isEnabled && event.type == Expose)
        needRedraw = true;

    return true;
}


void Selection::updateEnd()
{
    if (!isEnabled)
        return;

    if (needRedraw) {
        draw();
        XFlush(display);
        needRedraw = false;
    }
}


void Selection::updateBorderProperties()
{
    borderWidth = std::max<int>(
        1,
        static_cast<float>(baseBorderWidth)
            * getDpi(display) / baseDpi + 0.5f);

    // Limit to 127 since both XGCValues and XSetDashes() use the
    // plain char type for the dash length. We can work around this by
    // drawing the pattern manually instead of using XDrawLine() with
    // LineDoubleDash, but it doesn't make sense to bother since it's
    // unlikely that anyone would use a border this big, and even if
    // they do, shorter dashes will not affect usability.
    dashLen = std::min(127, borderWidth * squaresPerDash);

    const auto patternPhase = dashLen * 2;

    // To improve performance (especially when using small dashes),
    // make the tile reasonably large. We can use the screen size
    // instead of a constant number, but that would probably be
    // excessive since selecting the entire screen is uncommon.
    const auto minTileSize = 1000;

    auto tileSize = minTileSize;
    if (const auto rem = tileSize % patternPhase)
        tileSize += patternPhase - rem;

    const auto createPixmap = [&](int w, int h)
    {
        return PixmapHandle{
            display,
            XCreatePixmap(
                display, rootWindow,
                w, h, XDefaultDepth(display, screenNum))};
    };

    xTile = createPixmap(tileSize, borderWidth);
    yTile = createPixmap(borderWidth, tileSize);

    XGCValues gcval;
    gcval.line_width = borderWidth;
    gcval.dashes = dashLen;
    XChangeGC(display, tileGc, GCLineWidth | GCDashList, &gcval);

    const auto halfBw = borderWidth / 2;
    XDrawLine(display, xTile, tileGc, 0, halfBw, tileSize, halfBw);
    XDrawLine(display, yTile, tileGc, halfBw, 0, halfBw, tileSize);

    const auto setTile = [&](int borderIdx, Pixmap tile)
    {
        XSetTile(display, borderGcs[borderIdx], tile);
    };

    setTile(0, xTile);
    setTile(1, yTile);
    setTile(2, xTile);
    setTile(3, yTile);
}


void Selection::updateWindow()
{
    XMoveResizeWindow(
        display,
        window,
        geom.x - borderWidth,
        geom.y - borderWidth,
        geom.w + borderWidth * 2,
        geom.h + borderWidth * 2);

    const auto bw = borderWidth;
    const auto wBw = geom.w + bw;
    const auto hBw = geom.h + bw;

    const auto setBorderRect =
        [&](int borderIdx, int x, int y, int w, int h)
    {
        borderRects[borderIdx] = {
            static_cast<short>(x),
            static_cast<short>(y),
            static_cast<unsigned short>(w),
            static_cast<unsigned short>(h)};
    };

    setBorderRect(0, bw, 0, wBw, bw);
    setBorderRect(1, wBw, bw, bw, hBw);
    setBorderRect(2, 0, hBw, wBw, bw);
    setBorderRect(3, 0, 0, bw, hBw);

    XShapeCombineRectangles(
        display,
        window,
        ShapeBounding,
        0,
        0,
        borderRects,
        numBorders,
        ShapeSet,
        Unsorted);

    // Now we need to adjust the tile offsets so that separate
    // XFillRectangle() calls give the same contiguous dash pattern as
    // if it were drawn using a single XDrawRectangle() call.
    //
    // Keep in mind that in X11, the tile origin is the top-left
    // corner of the drawable.

    const auto patternPhase = dashLen * 2;

    // The alignment offsets are the base offsets needed to shift the
    // tile to the border rectangle. startAlignOffset is for the top
    // and right borders, where the pattern goes from start to end
    // along the drawing coordinates. endAlignOffset is for the bottom
    // and left borders, where the pattern goes backward; in this
    // case, we apply an extra shift by the dash length to visually
    // mirror the double-dash pattern.

    const auto startAlignOffset = borderWidth;
    const auto endAlignOffset = [&](int segmentLen)
    {
        return (segmentLen % patternPhase) + dashLen;
    };

    auto nextPatternOffset = [&, len = 0](int segmentLen) mutable
    {
        const auto offset = len % patternPhase;
        len += segmentLen;
        return offset;
    };

    const auto setTileOrigin = [&](int borderIdx, int x, int y)
    {
        XSetTSOrigin(display, borderGcs[borderIdx], x, y);
    };

    setTileOrigin(0, startAlignOffset - nextPatternOffset(wBw), 0);
    setTileOrigin(1, 0, startAlignOffset - nextPatternOffset(hBw));
    setTileOrigin(2, endAlignOffset(wBw) + nextPatternOffset(wBw), 0);
    setTileOrigin(3, 0, endAlignOffset(hBw) + nextPatternOffset(hBw));
}


void Selection::draw()
{
    for (int i{}; i < numBorders; ++i) {
        const auto& rect = borderRects[i];

        XFillRectangle(
            display, window, borderGcs[i],
            rect.x, rect.y, rect.width, rect.height);
    }
}


}
