
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


Rect Rect::betweenPoints(const Point& a, const Point& b)
{
    return {
        Side::betweenPoints(a.x, b.x), Side::betweenPoints(a.y, b.y)
    };
}


Side Side::getIntersection(const Side& other) const
{
    const auto min = std::max(start, other.start);
    const auto max = std::min(start + size, other.start + other.size);

    return {min, max > min ? max - min : 0};
}


Rect Rect::getIntersection(const Rect& other) const
{
    return {
        Side{x, w}.getIntersection({other.x, other.w}),
        Side{y, h}.getIntersection({other.y, other.h}),
    };
}


}
