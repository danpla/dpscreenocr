
#include "geometry.h"

#include <algorithm>


namespace dpso {
namespace {


struct Side {
    int start;
    int size;

    static Side betweenPoints(int a, int b)
    {
        if (a < b)
            return {a, b - a};

        return {b, a - b};
    }
};


}


static Side getIntersection(const Side& a, const Side& b)
{
    const auto min = std::max(a.start, b.start);
    const auto max = std::min(a.start + a.size, b.start + b.size);

    return {min, max > min ? max - min : 0};
}


static Rect makeRect(const Side& x, const Side& y)
{
    return {x.start, y.start, x.size, y.size};
}


Rect Rect::betweenPoints(const Point& a, const Point& b)
{
    return makeRect(
        Side::betweenPoints(a.x, b.x), Side::betweenPoints(a.y, b.y));
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
    return makeRect(
        getIntersection({a.x, a.w}, {b.x, b.w}),
        getIntersection({a.y, a.h}, {b.y, b.h}));
}


}
