
#pragma once


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

    static Rect betweenPoints(const Point& a, const Point& b);

    bool empty() const
    {
        return w <= 0 || h <= 0;
    }

    Rect getIntersection(const Rect& rect) const;
};


}
