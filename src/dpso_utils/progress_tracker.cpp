#include "progress_tracker.h"

#include <cassert>
#include <cmath>
#include <utility>


namespace dpso {


ProgressTracker::ProgressTracker(
        int numJobs,
        ProgressHandler progressHandler,
        float sensitivity)
    : ProgressTracker{numJobs}
{
    this->progressHandler = std::move(progressHandler);
    this->sensitivity = sensitivity;
}


ProgressTracker::ProgressTracker(
        int numJobs,
        ProgressTracker* parent)
    : ProgressTracker{numJobs}
{
    this->parent = parent;
}


ProgressTracker::ProgressTracker(int numJobs)
    : numJobs{numJobs > 0 ? numJobs : 1}
{
}


void ProgressTracker::advanceJob()
{
    ++curJobNum;
    assert(curJobNum <= numJobs);
    if (curJobNum > numJobs)
        curJobNum = numJobs;

    report((curJobNum - 1) / static_cast<float>(numJobs));
}


void ProgressTracker::update(float jobProgress)
{
    report((curJobNum - 1 + jobProgress) / numJobs);
}


void ProgressTracker::finish()
{
    report(1.0f);
}


void ProgressTracker::report(float progress)
{
    if (parent) {
        parent->update(progress);
        return;
    }

    if (progress != 1.0f && sensitivity > 0.0f)
        progress = std::floor(progress / sensitivity) * sensitivity;

    // Even if sensitivity is 0, we still check the values to avoid
    // invoking the handler when update() is called several times in
    // a row with the same progress, e.g. when a hierarchy of trackers
    // call advanceJob() for the first time, or when the child's
    // finish() is followed by the parent's advanceJob().
    if (progress <= lastProgress)
        return;

    if (progressHandler)
        progressHandler(progress);

    lastProgress = progress;
}


}
