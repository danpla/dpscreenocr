
#include "dpso/geometry.h"

#include "dpso_utils/str.h"

#include "flow.h"


using namespace dpso;


namespace {


std::string toStr(const Point& p)
{
    return str::format("Point{{{}, {}}}", p.x, p.y);
}


std::string toStr(const Size& s)
{
    return str::format("Size{{{}, {}}}", s.w, s.h);
}


std::string toStr(const Rect& r)
{
    return str::format("Rect{{{}, {}, {}, {}}}", r.x, r.y, r.w, r.h);
}


template<typename T>
void testEqual(const T& a, const T& b, int lineNum)
{
    if (a == b)
        return;

    test::failure("line {}: {} != {}\n", lineNum, toStr(a), toStr(b));
}


#define TEST_EQUAL(a, b) testEqual(a, b, __LINE__)


template<typename T>
void testEmpty(const T& v, bool expectEmpty, int lineNum)
{
    if (isEmpty(v) == expectEmpty)
        return;

    test::failure(
        "line {}: {} is expected to be {}empty\n",
        lineNum,
        toStr(v),
        expectEmpty ? "" : "non-");
}


#define TEST_EMPTY(v, expectEmpty) testEmpty(v, expectEmpty, __LINE__)


void testPoint()
{
    TEST_EQUAL(Point(), Point(0, 0));
}


void testSize()
{
    TEST_EQUAL(Size(), Size(0, 0));

    TEST_EMPTY(Size(), true);
    TEST_EMPTY(Size(-1, 0), true);
    TEST_EMPTY(Size(-1, 1), true);
    TEST_EMPTY(Size(0, -1), true);
    TEST_EMPTY(Size(1, -1), true);
    TEST_EMPTY(Size(1, 0), true);
    TEST_EMPTY(Size(0, 1), true);

    TEST_EMPTY(Size(1, 1), false);
}


void testRect()
{
    TEST_EQUAL(Rect(), Rect(0, 0, 0, 0));

    TEST_EQUAL(Rect({1, 2}, {3, 4}), Rect(1, 2, 3, 4));

    TEST_EQUAL(
        Rect::betweenPoints({0, 0}, Point{1, 1}), Rect(0, 0, 1, 1));
    TEST_EQUAL(
        Rect::betweenPoints({1, 1}, {0, 0}), Rect(0, 0, 1, 1));
    TEST_EQUAL(
        Rect::betweenPoints({-1, 1}, {1, -1}), Rect(-1, -1, 2, 2));
    TEST_EQUAL(
        Rect::betweenPoints({1, -1}, {-1, 1}), Rect(-1, -1, 2, 2));

    TEST_EMPTY(Rect(), true);
    TEST_EMPTY(Rect(0, 0, 1, 0), true);
    TEST_EMPTY(Rect(0, 0, -1, 0), true);
    TEST_EMPTY(Rect(0, 0, 0, 1), true);
    TEST_EMPTY(Rect(0, 0, 0, -1), true);
    TEST_EMPTY(Rect(0, 0, -1, -1), true);

    TEST_EMPTY(Rect(0, 0, 1, 1), false);

    TEST_EQUAL(getPos(Rect(1, 2, 3, 4)), Point(1, 2));
    TEST_EQUAL(getSize(Rect(1, 2, 3, 4)), Size(3, 4));

    const Rect r(0, 0, 2, 2);
    TEST_EQUAL(getIntersection(r, r), r);
    TEST_EQUAL(getIntersection(r, {}), Rect(0, 0, 0, 0));
    TEST_EQUAL(getIntersection(r, {1, 1, 2, 2}), Rect(1, 1, 1, 1));
    TEST_EQUAL(getIntersection(r, {2, 2, 2, 2}), Rect(2, 2, 0, 0));
    TEST_EQUAL(getIntersection(r, {3, 3, 2, 2}), Rect(3, 3, 0, 0));
    TEST_EQUAL(getIntersection(r, {-1, -1, 2, 2}), Rect(0, 0, 1, 1));
    TEST_EQUAL(getIntersection(r, {-2, -2, 2, 2}), Rect(0, 0, 0, 0));
    TEST_EQUAL(getIntersection(r, {-3, -3, 2, 2}), Rect(0, 0, 0, 0));
}


void testGeometry()
{
    testPoint();
    testSize();
    testRect();
}


}


REGISTER_TEST(testGeometry);
