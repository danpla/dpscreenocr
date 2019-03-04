
#include "test_geometry.h"

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
    if (r.empty() == expectEmpty)
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
    TEST_EQUAL(r.getIntersection(r), r);
    TEST_EQUAL(r.getIntersection({2, 2, 4, 4}), Rect(2, 2, 2, 2));
    TEST_EQUAL(r.getIntersection({4, 4, 4, 4}), Rect(4, 4, 0, 0));
    TEST_EQUAL(r.getIntersection({6, 6, 4, 4}), Rect(6, 6, 0, 0));
    TEST_EQUAL(r.getIntersection({-2, -2, 4, 4}), Rect(0, 0, 2, 2));
    TEST_EQUAL(r.getIntersection({-4, -4, 4, 4}), Rect(0, 0, 0, 0));
    TEST_EQUAL(r.getIntersection({-6, -6, 4, 4}), Rect(0, 0, 0, 0));
}


void testGeometry()
{
    testPoint();
    testRect();
}
