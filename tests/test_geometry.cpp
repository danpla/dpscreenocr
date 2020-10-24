
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "dpso/geometry.h"

#include "flow.h"


using namespace dpso;


static void testEqual(
    const Point& a, const Point& b,
    int lineNum)
{
    if (a.x == b.x && a.y == b.y)
        return;

    std::fprintf(
        stderr,
        "line %i: Point(%i, %i) != Point(%i, %i)\n",
        lineNum,
        a.x, a.y,
        b.x, b.y);
    test::failure();
}


static void testEqual(
    const Side& a, const Side& b,
    int lineNum)
{
    if (a.start == b.start && a.size == b.size)
        return;

    std::fprintf(
        stderr,
        "line %i: Size(%i, %i) != Size(%i, %i)\n",
        lineNum,
        a.start, a.size,
        b.start, b.size);
    test::failure();
}


static void testEqual(
    const Rect& a, const Rect& b,
    int lineNum)
{
    #define CMP(N) a.N == b.N
    if (CMP(x) && CMP(y) && CMP(w) && CMP(h))
        return;
    #undef CMP

    std::fprintf(
        stderr,
        "line %i: Rect(%i, %i, %i, %i) != Rect(%i, %i,  %i, %i)\n",
        lineNum,
        a.x, a.y, a.w, a.h,
        b.x, b.y, b.w, b.h);
    test::failure();
}


#define TEST_EQUAL(a, b) testEqual(a, b, __LINE__)


static void testEmpty(const Rect& r, bool expectEmpty, int lineNum)
{
    if (isEmpty(r) == expectEmpty)
        return;

    std::fprintf(
        stderr,
        "line %i: Rect(%i, %i, %i, %i) is expected to be %sempty\n",
        lineNum,
        r.x, r.y, r.w, r.h,
        expectEmpty ? "" : "non-");
    test::failure();
}


#define TEST_EMPTY(r, expectEmpty) \
    testEmpty(r, expectEmpty, __LINE__)


static void testPoint()
{
    TEST_EQUAL(Point(), Point(0, 0));
}


static void testSide()
{
    TEST_EQUAL(Side(), Side(0, 0));

    TEST_EQUAL(Side::betweenPoints(0, 1), Side(0, 1));
    TEST_EQUAL(Side::betweenPoints(1, 0), Side(0, 1));
    TEST_EQUAL(Side::betweenPoints(-1, 1), Side(-1, 2));
    TEST_EQUAL(Side::betweenPoints(1, -1), Side(-1, 2));

    const Side s(0, 4);
    TEST_EQUAL(getIntersection(s, s), s);
    TEST_EQUAL(getIntersection(s, {}), Side(0, 0));
    TEST_EQUAL(getIntersection(s, {2, 4}), Side(2, 2));
    TEST_EQUAL(getIntersection(s, {4, 4}), Side(4, 0));
    TEST_EQUAL(getIntersection(s, {6, 4}), Side(6, 0));
    TEST_EQUAL(getIntersection(s, {-2, 4}), Side(0, 2));
    TEST_EQUAL(getIntersection(s, {-4, 4}), Side(0, 0));
    TEST_EQUAL(getIntersection(s, {-6, 4}), Side(0, 0));
}


static void testRect()
{
    TEST_EQUAL(Rect(), Rect(0, 0, 0, 0));

    TEST_EQUAL(
        Rect::betweenPoints(Point(0, 0), Point(1, 1)),
        Rect(0, 0, 1, 1));
    TEST_EQUAL(
        Rect::betweenPoints(Point(1, 1), Point(0, 0)),
        Rect(0, 0, 1, 1));
    TEST_EQUAL(
        Rect::betweenPoints(Point(-1, 1), Point(1, -1)),
        Rect(-1, -1, 2, 2));
    TEST_EQUAL(
        Rect::betweenPoints(Point(1, -1), Point(-1, 1)),
        Rect(-1, -1, 2, 2));

    TEST_EMPTY(Rect(), true);
    TEST_EMPTY(Rect(0, 0, 1, 0), true);
    TEST_EMPTY(Rect(0, 0, -1, 0), true);
    TEST_EMPTY(Rect(0, 0, 0, 1), true);
    TEST_EMPTY(Rect(0, 0, 0, -1), true);
    TEST_EMPTY(Rect(0, 0, -1, -1), true);
    TEST_EMPTY(Rect(0, 0, 1, 1), false);

    const Rect r(0, 0, 4, 4);
    TEST_EQUAL(getIntersection(r, r), r);
    TEST_EQUAL(getIntersection(r, {}), Rect(0, 0, 0, 0));
    TEST_EQUAL(getIntersection(r, {2, 2, 4, 4}), Rect(2, 2, 2, 2));
    TEST_EQUAL(getIntersection(r, {4, 4, 4, 4}), Rect(4, 4, 0, 0));
    TEST_EQUAL(getIntersection(r, {6, 6, 4, 4}), Rect(6, 6, 0, 0));
    TEST_EQUAL(getIntersection(r, {-2, -2, 4, 4}), Rect(0, 0, 2, 2));
    TEST_EQUAL(getIntersection(r, {-4, -4, 4, 4}), Rect(0, 0, 0, 0));
    TEST_EQUAL(getIntersection(r, {-6, -6, 4, 4}), Rect(0, 0, 0, 0));
}


static void testGeometry()
{
    testPoint();
    testSide();
    testRect();
}


REGISTER_TEST("geometry", testGeometry);
