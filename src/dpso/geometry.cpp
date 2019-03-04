
#include "geometry.h"

#include <algorithm>


namespace dpso {


static inline void getSide(int a, int b, int& origin, int& size)
{
    if (a < b) {
        origin = a;
        size = b - a;
    } else {
        origin = b;
        size = a - b;
    }
}


Rect Rect::betweenPoints(const Point& a, const Point& b)
{
    Rect result;
    getSide(a.x, b.x, result.x, result.w);
    getSide(a.y, b.y, result.y, result.h);
    return result;
}


static inline void intersectSide(
    int aOrigin, int aSize, int bOrigin, int bSize,
    int& rOrigin, int& rSize)
{
    const auto start = std::max(aOrigin, bOrigin);
    const auto end = std::min(aOrigin + aSize, bOrigin + bSize);

    rOrigin = start;
    rSize = end > start ? end - start : 0;
}


Rect Rect::getIntersection(const Rect& rect) const
{
    Rect result;
    intersectSide(x, w, rect.x, rect.w, result.x, result.w);
    intersectSide(y, h, rect.y, rect.h, result.y, result.h);
    return result;
}


}
