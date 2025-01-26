#pragma once


struct DpsoRect;


namespace dpso {


struct Point {
    int x{};
    int y{};

    Point() = default;

    Point(int x, int y)
        : x{x}
        , y{y}
    {
    }
};


bool operator==(const Point& a, const Point& b);
bool operator!=(const Point& a, const Point& b);


struct Size {
    int w{};
    int h{};

    Size() = default;

    Size(int w, int h)
        : w{w}
        , h{h}
    {
    }
};


bool operator==(const Size& a, const Size& b);
bool operator!=(const Size& a, const Size& b);

bool isEmpty(const Size& size);


struct Rect {
    int x{};
    int y{};
    int w{};
    int h{};

    Rect() = default;

    Rect(int x, int y, int w, int h)
        : x{x}
        , y{y}
        , w{w}
        , h{h}
    {
    }

    Rect(const Point& pos, const Size& size)
        : Rect{pos.x, pos.y, size.w, size.h}
    {
    }

    explicit Rect(const DpsoRect& cRect);

    static Rect betweenPoints(const Point& a, const Point& b);
};


bool operator==(const Rect& a, const Rect& b);
bool operator!=(const Rect& a, const Rect& b);

Point getPos(const Rect& rect);
Size getSize(const Rect& rect);

DpsoRect toCRect(const Rect& rect);
bool isEmpty(const Rect& rect);
Rect getIntersection(const Rect& a, const Rect& b);


}
