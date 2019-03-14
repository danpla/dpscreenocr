
/**
 * \file
 * OCR routines
 *
 * Usage:
 *
 *   1. Activate at least one language with dpsoSetLangIsActive()
 *   2. Queue one or more jobs with dpsoQueueJob()
 *   3. Fetch job results:
 *     * Synchronously, with dpsoWaitForResults() followed by
 *         dpsoFetchResults()
 *     * Asynchronously, by calling dpsoFetchResults() repeatedly
 *         till all results are fetched
 *   4. Access the fetched results with dpsoGetFetchedResults()
 */

#pragma once

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get the number of available languages.
 */
int dpsoGetNumLangs(void);


/**
 * Get the language code.
 *
 * \param idx Language index [0, dpsoGetNumLangs())
 */
const char* dpsoGetLangCode(int langIdx);


/**
 * Get whether the language is active.
 *
 * \param idx Language index [0, dpsoGetNumLangs())
 */
int dpsoGetLangIsActive(int langIdx);


/**
 * Set whether the language is active.
 *
 * \param idx Language index [0, dpsoGetNumLangs())
 */
void dpsoSetLangIsActive(int langIdx, int newIsActive);


/**
 * Get the number of active languages.
 */
int dpsoGetNumActiveLangs(void);


typedef enum {
    /**
     * Automatic text segmentation.
     *
     * Tesseract will try to detect and split independent text
     * blocks, like text columns.
     */
    dpsoJobTextSegmentation = 1 << 0,

    /**
     * Dump debug image.
     *
     * Dump image that was passed to Tesseract to the current working
     * directory as "dpso_debug" + format-specific extension.
     */
    dpsoJobDumpDebugImage = 1 << 1
} DpsoJobFlag;


/**
 * Queue OCR job.
 *
 * The function captures an image (x, y, w, h) from the screen and
 * queues it for OCR using the currently active languages. The result
 * is 1 if the job is queued, or 0 if not. The latter can happen if
 * there are no active languages, or if the smaller side of the image
 * rectangle clamped to the screen is less than 5 px.
 *
 * Unfortunately, Tesseract only works with "C" locale. This locale
 * is automatically set on dpsoQueueJob() calls, and restored after
 * dpsoWaitForResults(), dpsoTerminateJobs(), and dpsoFetchResults()
 * when all jobs are completed. Don't change the locale between these
 * two points.
 */
int dpsoQueueJob(int x, int y, int w, int h, int jobFlags);


struct DpsoProgress {
    /**
     * Progress of the current job in percents (0-100).
     */
    int curJobProgress;

    /**
     * Number of the current job (1-based).
     *
     * Can be zero if a job (if any) is not yet started.
     */
    int curJob;

    /**
     * Total number of jobs.
     *
     * Can be zero if there are no pending jobs, meaning that all
     * jobs are completed, or no jobs was queued after dpsoInit().
     */
    int totalJobs;
};


/**
 * Get jobs progress.
 *
 * Be aware that since jobs are processed in the background, the
 * progress is not reliable. It can change even before this function
 * returns.
 *
 * isNew is 1 if the progress was changed from the last time
 * dpsoGetProgress() was called, or 0 otherwise. Keep in mind that 1
 * doesn't imply that the returned struct will actually differ from
 * the previous; they may be the same in case the progress refers to
 * a new job chain.
 *
 * If you just need to test if there are pending jobs, consider using
 * dpsoGetJobsPending().
 */
void dpsoGetProgress(struct DpsoProgress* progress, int* isNew);


/**
 * Check if there are pending jobs.
 *
 * The function returns 1 if there are pending jobs (that is, there
 * is an active or at least one queued job), 0 otherwise. This is
 * basically the same as testing DpsoProgress::totalJobs, except it
 * doesn't affect the isNew value of dpsoGetProgress().
 */
int dpsoGetJobsPending(void);


/**
 * Fetch job results.
 *
 * The function fetches the results of completed jobs to the internal
 * array you can access with dpsoGetFetchedResults().
 *
 * If fetchChain is 0, the function will fetch any currently
 * available results, even if not all jobs are completed. Use this if
 * you need to get results as soon as possible, but be aware that
 * since jobs are processed in the background, new results may arrive
 * even before this function returns. If fetchChain is not 0, the
 * function only fetches results if all jobs are completed, that is,
 * there are neither active nor queued jobs. In this case, you can be
 * sure that there will be no new results unless dpsoQueueJob() is
 * called after dpsoFetchResults().
 *
 * The function returns 1 if at least one result is fetched, or 0
 * otherwise, in which case the previously fetched results remain
 * valid.
 *
 * \sa dpsoGetResult()
 */
int dpsoFetchResults(int fetchChain);


/**
 * Result of a single OCR job.
 */
struct DpsoJobResult {
    /**
     * Null-terminated text in UTF-8 encoding.
     *
     * The text, if not empty, will have a trailing newline.
     */
    const char* text;

    /**
     * Length of the text, excluding the null terminator.
     *
     * The same as strlen(text).
     */
    size_t textLen;

    /**
     * Timestamp in ISO 8601 format.
     *
     * For example, "2018-12-03 19:21:24".
     */
    const char* timestamp;
};


/**
 * Get the results previously fetched with dpsoFetchResults().
 *
 * The results pointer is valid till the next call to
 * dpsoFetchResults() that returns 1. Initially, the array is empty.
 */
void dpsoGetFetchedResults(
    const struct DpsoJobResult** results, int* numResults);


/**
 * Progress callback for dpsoWaitForResults().
 *
 * To get the progress, use dpsoGetProgress(). You can also call
 * dpsoTerminateJobs() from the callback to terminate jobs.
 */
typedef void (*DpsoProgressCallback)(void* userData);


/**
 * Wait for results.
 *
 * The function blocks the caller's thread of execution till all jobs
 * are completed.
 *
 * The progress callback (may be null) is called every time the
 * progress changes. Use dpsoGetProgress() to get the progress.
 * You can also call dpsoTerminateOcr() from the callback to
 * terminate jobs prematurely.
 */
void dpsoWaitForResults(
    DpsoProgressCallback progressCallback, void* userData);


/**
 * Terminate jobs.
 *
 * This function terminates the active job, clears the job queue, and
 * drops all unfetched job results. dpsoTerminateJobs() is implicitly
 * called on dpsoShutdown().
 */
void dpsoTerminateJobs(void);


#ifdef __cplusplus
}
#endif

