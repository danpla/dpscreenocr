
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


Side getIntersection(const Side& a, const Side& b)
{
    const auto min = std::max(a.start, b.start);
    const auto max = std::min(a.start + a.size, b.start + b.size);

    return {min, max > min ? max - min : 0};
}


Rect makeRect(const Side& x, const Side& y)
{
    return {x.start, y.start, x.size, y.size};
}


}


bool operator==(const Point& a, const Point& b)
{
    return a.x == b.x && a.y == b.y;
}


bool operator!=(const Point& a, const Point& b)
{
    return !(a == b);
}


bool operator==(const Size& a, const Size& b)
{
    return a.w == b.w && a.h == b.h;
}


bool operator!=(const Size& a, const Size& b)
{
    return !(a == b);
}


bool isEmpty(const Size& size)
{
    return size.w <= 0 || size.h <= 0;
}


bool operator==(const Rect& a, const Rect& b)
{
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}


bool operator!=(const Rect& a, const Rect& b)
{
    return !(a == b);
}


Rect Rect::betweenPoints(const Point& a, const Point& b)
{
    return makeRect(
        Side::betweenPoints(a.x, b.x), Side::betweenPoints(a.y, b.y));
}


Point getPos(const Rect& rect)
{
    return {rect.x, rect.y};
}


Size getSize(const Rect& rect)
{
    return {rect.w, rect.h};
}


DpsoRect toCRect(const Rect& rect)
{
    return {rect.x, rect.y, rect.w, rect.h};
}


bool isEmpty(const Rect& rect)
{
    return isEmpty(getSize(rect));
}


Rect getIntersection(const Rect& a, const Rect& b)
{
    return makeRect(
        getIntersection({a.x, a.w}, {b.x, b.w}),
        getIntersection({a.y, a.h}, {b.y, b.h}));
}


}
