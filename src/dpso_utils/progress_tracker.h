
#pragma once

#include <functional>


namespace dpso {


/**
 * Progress tracker.
 *
 * The progress tracker allows reporting progress of an operation
 * consisting of several routines that don't aware of each other.
 *
 * The progress tracker operates on a fixed number of progress jobs.
 * A progress job is either an actual routine that reports its
 * progress directly to the tracker via update(), or a child tracker.
 * The tracker scales the total progress from all jobs to the range
 * from 0.0 to 1.0. For example, if the tracker has 2 jobs and the
 * first one reports 0.5, the value sent to the handler is 0.25.
 *
 * There are three types of trackers:
 *
 * * Toplevel - a tracker that reports progress to a progress handler.
 *
 * * Child - a tracker that reports progress to the parent's update()
 *   method. It acts like a group and is treated as a single job by
 *   the parent.
 *
 * * Null - a tracker that does nothing. You can create it as a child
 *   with a null parent, or explicitly using the constructor that only
 *   takes the number of jobs.
 */
class ProgressTracker {
public:
    /**
     * Progress update.
     *
     * The progress is in range 0.0 - 1.0.
     */
    using ProgressHandler = std::function<void(float progress)>;

    /**
     * Create top-level tracker.
     *
     * \param numJobs Total number of progress jobs.
     * \param progressHandler Progress handler.
     * \param sensitivity How often to invoke the progress handler.
     *     The smaller the value, the more often the handler is
     *     called. For example, with the default value (0.01) the
     *     progress is reported no more often than every 1%.
     */
    ProgressTracker(
        int numJobs,
        ProgressHandler progressHandler,
        float sensitivity = 0.01);

    /**
     * Create child tracker.
     *
     * \param numJobs Total number of progress jobs.
     * \param parent Parent tracker. May be null to create a null
     *     tracker.
     */
    ProgressTracker(int numJobs, ProgressTracker* parent);

    /**
     * Create null tracker.
     *
     * \param numJobs Total number of progress jobs.
     */
    explicit ProgressTracker(int numJobs);

    /**
     * Advance to the next job.
     *
     * The jobs are counted from 1. The initial number of the current
     * job is 0, which means you should call advanceJob() before using
     * update().
     *
     * The total number of jobs will be clamped to the maximum
     * specified in the constructor. At the same time, this method has
     * an assert() call to help you detect errors.
     */
    void advanceJob();

    /**
     * Update progress of the current job.
     *
     * \param jobProgress Progress from 0.0 to 1.0.
     */
    void update(float jobProgress);

    /**
     * Finish progress.
     *
     * finish() unconditionally reports 1.0.
     */
    void finish();
private:
    int numJobs;
    ProgressHandler progressHandler;
    ProgressTracker* parent;
    float sensitivity;

    int curJobNum;
    float lastProgress;

    void report(float progress);
};


}
