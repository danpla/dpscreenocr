
#include "dpso/geometry.h"

#include <fmt/core.h>

#include "flow.h"


using namespace dpso;


namespace {


std::string toStr(const Point& p)
{
    return fmt::format("Point{{{}, {}}}", p.x, p.y);
}


void testEqual(const Point& a, const Point& b, int lineNum)
{
    if (a.x == b.x && a.y == b.y)
        return;

    test::failure("line {}: {} != {}\n", lineNum, toStr(a), toStr(b));
}


std::string toStr(const Rect& r)
{
    return fmt::format("Rect{{{}, {}, {}, {}}}", r.x, r.y, r.w, r.h);
}


void testEqual(const Rect& a, const Rect& b, int lineNum)
{
    #define CMP(N) a.N == b.N
    if (CMP(x) && CMP(y) && CMP(w) && CMP(h))
        return;
    #undef CMP

    test::failure("line {}: {} != {}\n", lineNum, toStr(a), toStr(b));
}


#define TEST_EQUAL(a, b) testEqual(a, b, __LINE__)


void testEmpty(const Rect& r, bool expectEmpty, int lineNum)
{
    if (isEmpty(r) == expectEmpty)
        return;

    test::failure(
        "line {}: {} is expected to be {}empty\n",
        lineNum,
        toStr(r),
        expectEmpty ? "" : "non-");
}


#define TEST_EMPTY(r, expectEmpty) testEmpty(r, expectEmpty, __LINE__)


void testPoint()
{
    TEST_EQUAL(Point(), Point(0, 0));
}


void testRect()
{
    TEST_EQUAL(Rect(), Rect(0, 0, 0, 0));

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
    testRect();
}


}


REGISTER_TEST(testGeometry);
