
#include "dpso/progress_tracker.h"

#include "flow.h"

#include <cmath>


namespace {


struct ProgressInfo {
    int numCalls;
    float progress;
};


}


static void updateProgressInfo(float progress, void* userData)
{
    auto& pt = *static_cast<ProgressInfo*>(userData);
    ++pt.numCalls;
    pt.progress = progress;
}


static void cmpProgressInfo(
    const ProgressInfo& a, const ProgressInfo& b, int lineNum)
{
    if (a.numCalls != b.numCalls)
        test::failure(
            "line %i: ProgressInfo::numCalls don't match: %i != %i\n",
            lineNum,
            a.numCalls,
            b.numCalls);

    if (std::abs(a.progress - b.progress) > 0.00001f)
        test::failure(
            "line %i: ProgressInfo::progress don't match: %f != %f\n",
            lineNum,
            a.progress,
            b.progress);
}


#define CHECK_PROGRESS_INFO(NUM_CALLS, PROGRESS) \
    cmpProgressInfo( \
        progressInfo, ProgressInfo{NUM_CALLS, PROGRESS}, __LINE__)


static void testHierarchy()
{
    ProgressInfo progressInfo{0, -1.0f};

    dpso::ProgressTracker toplevelPt{
        2, updateProgressInfo, &progressInfo, 0.0f};
    dpso::ProgressTracker childPt{2, &toplevelPt};
    dpso::ProgressTracker grandChildPt{2, &childPt};

    CHECK_PROGRESS_INFO(0, -1.0f);

    toplevelPt.advanceJob();
    childPt.advanceJob();
    grandChildPt.advanceJob();

    CHECK_PROGRESS_INFO(1, 0.0f);

    grandChildPt.update(0.5f);
    CHECK_PROGRESS_INFO(2, 0.0625f);

    grandChildPt.advanceJob();
    CHECK_PROGRESS_INFO(3, 0.125f);

    grandChildPt.update(0.5f);
    CHECK_PROGRESS_INFO(4, 0.1875f);

    grandChildPt.finish();
    CHECK_PROGRESS_INFO(5, 0.25f);

    childPt.advanceJob();
    CHECK_PROGRESS_INFO(5, 0.25f);

    childPt.update(0.5f);
    CHECK_PROGRESS_INFO(6, 0.375f);

    childPt.finish();
    CHECK_PROGRESS_INFO(7, 0.5f);

    // Skip advanceJob();
    toplevelPt.finish();
    CHECK_PROGRESS_INFO(8, 1.0f);
}


static void testSensitivity()
{
    ProgressInfo progressInfo{0, -1.0f};

    dpso::ProgressTracker toplevelPt{
        2, updateProgressInfo, &progressInfo};
    dpso::ProgressTracker childPt{2, &toplevelPt};

    CHECK_PROGRESS_INFO(0, -1.0f);

    toplevelPt.advanceJob();
    childPt.advanceJob();

    CHECK_PROGRESS_INFO(1, 0.0f);

    childPt.update(0.05f); // 0.0125
    CHECK_PROGRESS_INFO(2, 0.01f);

    childPt.update(0.079f); // 0.01975
    CHECK_PROGRESS_INFO(2, 0.01f);

    childPt.update(0.08f); // 0.02
    CHECK_PROGRESS_INFO(3, 0.02f);

    childPt.advanceJob();
    CHECK_PROGRESS_INFO(4, 0.25f);

    childPt.finish();
    toplevelPt.advanceJob();
    CHECK_PROGRESS_INFO(5, 0.5f);

    toplevelPt.update(0.99999);
    CHECK_PROGRESS_INFO(6, 0.99f);

    toplevelPt.finish();
    CHECK_PROGRESS_INFO(7, 1.0f);
}


static void testProgressTracker()
{
    testHierarchy();
    testSensitivity();
}


REGISTER_TEST(testProgressTracker);
