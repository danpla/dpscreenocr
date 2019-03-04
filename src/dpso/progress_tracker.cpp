
#include "progress_tracker.h"

#include <cassert>


namespace dpso {


static void nullProgressFn(float progress, void* userData)
{
    (void)progress;
    (void)userData;
}


ProgressTracker::ProgressTracker(
        int numJobs,
        ProgressFn progressFn,
        void* userData,
        float sensitivity)
    : ProgressTracker(numJobs)
{
    this->progressFn = progressFn ? progressFn : nullProgressFn;
    this->userData = userData;
    this->sensitivity = sensitivity;
}


ProgressTracker::ProgressTracker(
        int numJobs,
        ProgressTracker* parent)
    : ProgressTracker(numJobs)
{
    this->parent = parent;
}


ProgressTracker::ProgressTracker(int numJobs)
    : numJobs {numJobs > 0 ? numJobs : 1}
    , progressFn {nullProgressFn}
    , userData {}
    , parent {}
    , sensitivity {}
    , jobProgressScale {1.0f / this->numJobs}
    , baseProgress {}
    , curJobNum {}
    , curProgress {}
{

}


void ProgressTracker::start()
{
    reset();
    report(0.0f);
}


void ProgressTracker::advanceJob(int count)
{
    if (count < 1)
        return;

    curJobNum += count;
    assert(curJobNum <= numJobs);
    if (curJobNum > numJobs)
        curJobNum = numJobs;

    baseProgress = jobProgressScale * (curJobNum - 1);
}


void ProgressTracker::update(float jobProgress)
{
    const auto newCurProgress = (
        baseProgress + jobProgress * jobProgressScale);

    if (newCurProgress - curProgress < sensitivity)
        return;

    curProgress = newCurProgress;
    report(curProgress);
}


void ProgressTracker::finish()
{
    report(1.0f);
}


void ProgressTracker::reset()
{
    baseProgress = {};
    curJobNum = {};
    curProgress = {};
}


void ProgressTracker::report(float progress)
{
    if (parent)
        parent->update(progress);
    else
        progressFn(progress, userData);
}


}
