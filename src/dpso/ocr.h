
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

#include "geometry_c.h"


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
 * Returns an empty string if langIdx is out of bounds.
 *
 * \param idx Language index [0, dpsoGetNumLangs())
 */
const char* dpsoGetLangCode(int langIdx);


/**
 * Get default language code.
 *
 * The main purpose of the default language code is to select
 * a language when a program starts for the first time.
 *
 * Returns an empty string if the OCR engine has no meaningful default
 * language.
 */
const char* dpsoGetDefaultLangCode(void);


/**
 * Get language name.
 *
 * Returns null if the language name for the given code is not known.
 */
const char* dpsoGetLangName(const char* langCode);


/**
 * Get language index.
 *
 * Returns -1 if the language with the given code is not available.
 */
int dpsoGetLangIdx(const char* langCode);


/**
 * Get whether the language is active.
 *
 * Returns 0 if langIdx is out of bounds.
 *
 * \param idx Language index [0, dpsoGetNumLangs())
 */
int dpsoGetLangIsActive(int langIdx);


/**
 * Set whether the language is active.
 *
 * Does nothing if langIdx is out of bounds.
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
     * Try to detect and split independent text blocks, like columns.
     */
    dpsoJobTextSegmentation = 1 << 0
} DpsoJobFlag;


/**
 * OCR job arguments.
 */
struct DpsoJobArgs {
    /**
     * The rectangle to capture from screen for recognition.
     */
    struct DpsoRect screenRect;

    /**
     * Combination of DpsoJobFlag flags.
     */
    unsigned flags;
};


/**
 * Queue OCR job.
 *
 * The function captures an image DpsoJobArgs::screenRect from the
 * screen and queues it for OCR using the currently active languages.
 * The result is 1 if the job is queued, or 0 if not. The latter
 * happens either if there are no active languages, or if screenRect
 * is empty after clamping to the screen.
 *
 * Unfortunately, Tesseract versions before 4.1.0 only work with "C"
 * locale. This locale is automatically set on dpsoQueueJob() calls,
 * and restored after dpsoWaitForResults(), dpsoTerminateJobs(), and
 * dpsoFetchResults() when all jobs are completed. Don't change the
 * locale between these two points.
 */
int dpsoQueueJob(const struct DpsoJobArgs* jobArgs);


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


typedef enum {
    /**
     * Fetch currently available results.
     *
     * dpsoFetchResults() will fetch any currently available results,
     * even if not all jobs are completed. Use this if you need to get
     * results as soon as possible, but be aware that since jobs are
     * processed in the background, new results may arrive even
     * before the function returns.
     */
    dpsoFetchCurentlyAvailable,

    /**
     * Fetch full result chain.
     *
     * dpsoFetchResults() will only fetch results if all jobs are
     * completed, that is, there are neither active nor queued jobs.
     * In this case, you can be sure that there will be no new results
     * unless dpsoQueueJob() is called after dpsoFetchResults().
     */
    dpsoFetchFullChain
} DpsoResultFetchingMode;


/**
 * Fetch job results.
 *
 * The function fetches the results of completed jobs to the internal
 * array you can access with dpsoGetFetchedResults().
 *
 * The function returns 1 if at least one result is fetched, or 0
 * otherwise, in which case the previously fetched results remain
 * valid.
 *
 * \sa DpsoResultFetchingMode
 * \sa dpsoGetFetchedResults()
 */
int dpsoFetchResults(DpsoResultFetchingMode fetchingMode);


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
     * Timestamp in "YYYY-MM-DD hh:mm:ss" format.
     */
    const char* timestamp;
};


struct DpsoJobResults {
    const struct DpsoJobResult* items;
    int numItems;
};


/**
 * Get the results previously fetched with dpsoFetchResults().
 *
 * The items pointer is valid till the next call to dpsoFetchResults()
 * that returns 1. Initially, the array is empty.
 */
void dpsoGetFetchedResults(struct DpsoJobResults* results);


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
 * You can also call dpsoTerminateJobs() from the callback to
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
