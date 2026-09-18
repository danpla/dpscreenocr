#pragma once

#include "dpso_utils/geometry.h"


namespace dpso::backend {


// Make a selection rectangle from two points: the initial and the
// current mouse cursor positions. The function accepts them in any
// order. Since the mouse cursor position always refers to a pixel,
// the returned rectangle will never be empty: even identical points
// will result in a rectangle enclosing a single pixel.
inline Rect makeSelectionRect(const Point& a, const Point& b)
{
    auto rect = Rect::betweenPoints(a, b);
    ++rect.w;
    ++rect.h;
    return rect;
}


}
