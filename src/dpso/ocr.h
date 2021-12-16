
/**
 * \file
 * OCR routines
 *
 * Usage:
 *
 *   1. Activate at least one language with dpsoSetLangIsActive()
 *   2. Queue one or more jobs with dpsoQueueJob()
 *   3. Fetch job results:
 *     * Synchronously, with dpsoWaitJobsToComplete() followed by
 *         dpsoFetchResults()
 *     * Asynchronously, by calling dpsoFetchResults() repeatedly
 *         till all results are fetched
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
 * screen and queues it for OCR with the currently active languages.
 * On failure, sets an error message (dpsoGetError()) and returns 0.
 * Reasons include:
 *   * jobArgs is null
 *   * jobArgs::screenRect is empty or outside the screen bounds
 *   * No active languages
 *   * Error when taking a screenshot
 *
 * Tesseract versions before 4.1.0 only work with "C" locale. This
 * locale is automatically set on a successful dpsoQueueJob() call,
 * and restored after dpsoWaitJobsToComplete(), dpsoTerminateJobs(),
 * or dpsoFetchResults() when all jobs are completed. Don't change the
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
 * Return 1 if two DpsoProgress are equal, 0 otherwise.
 */
int dpsoProgressEqual(
    const struct DpsoProgress* a, const struct DpsoProgress* b);


/**
 * Get jobs progress.
 *
 * If you just need to test if there are pending jobs, consider using
 * dpsoGetJobsPending().
 */
void dpsoGetProgress(struct DpsoProgress* progress);


/**
 * Check if there are pending jobs.
 *
 * The function returns 1 if there are pending jobs (that is, there
 * is an active or at least one queued job), 0 otherwise. This is
 * basically the same as testing DpsoProgress::totalJobs.
 */
int dpsoGetJobsPending(void);


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


/**
 * Reference to internal array containing OCR job results.
 *
 * The results remain valid till the next dpsoFetchResults() or
 * dpsoShutdown() call.
 *
 * \sa dpsoFetchResults
 */
struct DpsoJobResults {
    const struct DpsoJobResult* items;
    int numItems;
};


/**
 * Fetch job results.
 *
 * The function returns a reference to an internal array filled with
 * results of completed OCR jobs; if there are no new completed jobs,
 * the array will be empty. The previously returned DpsoJobResults
 * gets invalidated.
 */
void dpsoFetchResults(struct DpsoJobResults* results);


/**
 * Progress callback for dpsoWaitJobsToComplete().
 *
 * You can use dpsoGetProgress() inside the callback to get the actual
 * progress, and dpsoTerminateJobs() to terminate jobs prematurely.
 */
typedef void (*DpsoProgressCallback)(void* userData);


/**
 * Block the current thread till all jobs are completed.
 *
 * The progress callback (may be null) is called every time the
 * progress changes.
 */
void dpsoWaitJobsToComplete(
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
