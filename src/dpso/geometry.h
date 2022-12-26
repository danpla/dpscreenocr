
#pragma once

#include "geometry_c.h"


namespace dpso {


struct Point {
    int x;
    int y;

    Point()
        : Point{0, 0}
    {
    }

    Point(int x, int y)
        : x{x}
        , y{y}
    {
    }
};


struct Rect {
    int x;
    int y;
    int w;
    int h;

    Rect()
        : Rect{0, 0, 0, 0}
    {
    }

    Rect(int x, int y, int w, int h)
        : x{x}
        , y{y}
        , w{w}
        , h{h}
    {
    }

    explicit Rect(const DpsoRect& cRect)
        : Rect{cRect.x, cRect.y, cRect.w, cRect.h}
    {
    }

    static Rect betweenPoints(const Point& a, const Point& b);
};


DpsoRect toCRect(const Rect& rect);
bool isEmpty(const Rect& rect);
Rect getIntersection(const Rect& a, const Rect& b);


}
