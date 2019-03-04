
#pragma once


namespace dpso {


/**
 * Progress tracker.
 *
 * The progress tracker solves the problem of reporting progress of
 * some operation that consists of several routines that don't aware
 * of each other.
 *
 * The progress tracker operates on a fixed number of progress jobs.
 * A progress job is either an actual routine that reports it's
 * progress directly to the tracker via update(), or a child tracker.
 * The tracker scales the total progress from all jobs to the range
 * from 0.0 to 1.0. For example, if the tracker has 2 jobs and the
 * first one reports 0.5, the value sent to the callback is 0.25.
 *
 * There are three types of trackers:
 *
 *   * Toplevel - a tracker that reports progress via a callback.
 *
 *   * Child - a tracker that reports progress to the parent's
 *     update() method. That way, the child tracker acts like a group;
 *     it's treated as a single job by the parent.
 *
 *   * Null - a tracker that does nothing. You can create it as a
 *     toplevel with a null callback, as a child with a null parent,
 *     or explicitly using the constructor that only takes the number
 *     of jobs.
 */
class ProgressTracker {
public:
    /**
     * Progress callback.
     *
     * \param progress Progress from 0.0 to 1.0.
     * \param userData User data
     */
    using ProgressFn = void (*)(float progress, void* userData);

    /**
     * Create top-level tracker.
     *
     * \param numJobs Total numbers of progress jobs.
     * \param progresFn Progress callback.
     * \param userData User data for the progress callback.
     * \param sensitivity How often to invoke the progress callback.
     *     The smaller the value, the more often the callback is
     *     called. For example, with the default value (0.01) the
     *     progress is reported every 1%.
     */
    ProgressTracker(
        int numJobs,
        ProgressFn progressFn,
        void* userData = nullptr,
        float sensitivity = 0.01);

    /**
     * Create child tracker.
     *
     * \param numJobs Total numbers of progress jobs.
     * \param parent Pointer to the parent tracker, or null to create
     *     a null tracker.
     */
    ProgressTracker(int numJobs, ProgressTracker* parent);

    /**
     * Create null tracker.
     *
     * \param numJobs Total numbers of progress jobs.
     */
    explicit ProgressTracker(int numJobs);

    /**
     * Start progress.
     *
     * start() resets the progress tracker and unconditionally
     * reports 0.0 progress. Calling this method is optional, unless
     * you want to reuse the tracker.
     */
    void start();

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
     *
     * \param count Number of jobs to advance. May be > 1 if you
     *     need to skip some. Values < 1 are ignored.
     */
    void advanceJob(int count = 1);

    /**
     * Update progress of the current job.
     *
     * \param jobProgress Progress from 0.0 to 1.0.
     */
    void update(float jobProgress);

    /**
     * Finish progress.
     *
     * finish() unconditionally reports 1.0 progress. Be aware that it
     * doesn't reset the state of the tracker; use start() for that.
     */
    void finish();
private:
    int numJobs;
    ProgressFn progressFn;
    void* userData;
    ProgressTracker* parent;
    float sensitivity;

    float jobProgressScale;

    float baseProgress;
    int curJobNum;
    float curProgress;

    void reset();
    void report(float progress);
};


}
