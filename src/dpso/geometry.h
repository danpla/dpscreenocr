
#pragma once

#include "geometry_c.h"


namespace dpso {


struct Point {
    int x;
    int y;

    Point()
        : Point(0, 0)
    {

    }

    Point(int x, int y)
        : x {x}
        , y {y}
    {

    }
};


struct Side {
    int start;
    int size;

    Side()
        : Side(0, 0)
    {

    }

    Side(int start, int size)
        : start {start}
        , size {size}
    {

    }

    static Side betweenPoints(int a, int b);

    Side getIntersection(const Side& other) const;
};


struct Rect {
    int x;
    int y;
    int w;
    int h;

    Rect()
        : Rect(0, 0, 0, 0)
    {

    }

    Rect(int x, int y, int w, int h)
        : x {x}
        , y {y}
        , w {w}
        , h {h}
    {

    }

    Rect(const Side& x, const Side& y)
        : Rect(x.start, y.start, x.size, y.size)
    {

    }

    explicit Rect(const DpsoRect& cRect)
        : Rect(cRect.x, cRect.y, cRect.w, cRect.h)
    {

    }

    DpsoRect toCRect() const
    {
        return {x, y, w, h};
    }

    static Rect betweenPoints(const Point& a, const Point& b);

    bool empty() const
    {
        return w <= 0 || h <= 0;
    }

    Rect getIntersection(const Rect& other) const;
};


}
