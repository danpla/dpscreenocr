#pragma once


namespace dpso::img {


enum class Axis {
    x,
    y
};


constexpr Axis getOpposite(Axis axis)
{
    return axis == Axis::x ? Axis::y : Axis::x;
}


template<Axis axis>
int getSize(int w, int h)
{
    return axis == Axis::x ? w : h;
}


template<Axis axis, typename Px>
class Line {
public:
    Line(int n, Px* data, int pitch)
        : line{data + (axis == Axis::x ? n * pitch : n)}
        , pitch{pitch}
    {
    }

    Px& operator[](int px) const
    {
        return line[axis == Axis::x ? px : px * pitch];
    }
private:
    Px* line;
    int pitch;
};


template<Axis axis, typename Px>
auto makeLine(int n, Px* data, int pitch)
{
    return Line<axis, Px>{n, data, pitch};
}


}
