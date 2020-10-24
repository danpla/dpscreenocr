
#include "geometry.h"

#include <algorithm>


namespace dpso {


Side Side::betweenPoints(int a, int b)
{
    if (a < b)
        return {a, b - a};
    else
        return {b, a - b};
}


Side getIntersection(const Side& a, const Side& b)
{
    const auto min = std::max(a.start, b.start);
    const auto max = std::min(a.start + a.size, b.start + b.size);

    return {min, max > min ? max - min : 0};
}


Rect Rect::betweenPoints(const Point& a, const Point& b)
{
    return {
        Side::betweenPoints(a.x, b.x), Side::betweenPoints(a.y, b.y)
    };
}


DpsoRect toCRect(const Rect& rect)
{
    return {rect.x, rect.y, rect.w, rect.h};
}


bool isEmpty(const Rect& rect)
{
    return rect.w <= 0 || rect.h <= 0;
}


Rect getIntersection(const Rect& a, const Rect& b)
{
    return {
        getIntersection({a.x, a.w}, {b.x, b.w}),
        getIntersection({a.y, a.h}, {b.y, b.h})
    };
}


}
