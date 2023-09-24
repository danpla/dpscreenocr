
#include <cmath>

#include "dpso_utils/progress_tracker.h"

#include "flow.h"


namespace {


struct ProgressInfo {
    int numCalls{};
    float progress{-1.0f};
};


dpso::ProgressTracker::ProgressHandler makeProgressHandler(
    ProgressInfo& progressInfo)
{
    return [&](float progress)
        {
            ++progressInfo.numCalls;
            progressInfo.progress = progress;
        };
}


void checkProgressInfo(
    const ProgressInfo& progressInfo,
    int numCalls,
    float progress,
    int lineNum)
{
    if (progressInfo.numCalls != numCalls)
        test::failure(
            "line %i: ProgressInfo::numCalls{%i} != %i\n",
            lineNum, progressInfo.numCalls, numCalls);

    if (std::abs(progressInfo.progress - progress) > 0.00001f)
        test::failure(
            "line %i: ProgressInfo::progress{%f} != %f\n",
            lineNum, progressInfo.progress, progress);
}


#define CHECK_PROGRESS_INFO(NUM_CALLS, PROGRESS) \
    checkProgressInfo(progressInfo, NUM_CALLS, PROGRESS, __LINE__)


void testHierarchy()
{
    ProgressInfo progressInfo;

    dpso::ProgressTracker toplevelPt{
        2, makeProgressHandler(progressInfo), 0.0f};
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


void testSensitivity()
{
    ProgressInfo progressInfo;

    dpso::ProgressTracker toplevelPt{
        2, makeProgressHandler(progressInfo)};
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


void testProgressTracker()
{
    testHierarchy();
    testSensitivity();
}


}


REGISTER_TEST(testProgressTracker);
