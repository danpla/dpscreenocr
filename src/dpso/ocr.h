
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "geometry_c.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct DpsoOcr DpsoOcr;


/**
 * Create OCR.
 *
 * engineIdx is in the [0, dpsoOcrGetNumEngines()) range. dataDir may
 * be empty to use the default path, or if the engine doesn't use
 * external data. See DpsoOcrEngineDataDirPreference for the details.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * null. In particular, the function will fail if a language manager
 * for the same engine and data dir is active.
 */
DpsoOcr* dpsoOcrCreate(int engineIdx, const char* dataDir);


void dpsoOcrDelete(DpsoOcr* ocr);


/**
 * Get the number of available languages.
 */
int dpsoOcrGetNumLangs(const DpsoOcr* ocr);


/**
 * Get language code.
 *
 * A language code is a string that uniquely identify a language,
 * like an ISO 639 code. It contains only ASCII alphanumeric
 * characters, hyphens, and underscores.
 *
 * The languages are sorted alphabetically by their codes. If you need
 * to find a language index by its code, use dpsoOcrGetLangIdx()
 * instead of linear search.
 *
 * Returns an empty string if langIdx is out of
 * [0, dpsoOcrGetNumLangs()).
 */
const char* dpsoOcrGetLangCode(const DpsoOcr* ocr, int langIdx);


/**
 * Get default language code.
 *
 * The main purpose of default language code is to be used in GUI to
 * select a language when a program starts for the first time.
 *
 * The default language is normally English, but may be different
 * in case the OCR engine is designed for a specific group of
 * languages.
 *
 * Returns an empty string if the OCR engine has no meaningful default
 * language.
 */
const char* dpsoOcrGetDefaultLangCode(const DpsoOcr* ocr);


/**
 * Get language name.
 *
 * Returns an empty string if the language has no name, or if langIdx
 * is out of [0, dpsoOcrGetNumLangs()).
 */
const char* dpsoOcrGetLangName(const DpsoOcr* ocr, int langIdx);


/**
 * Get language index.
 *
 * The function uses binary search. Returns -1 if the language with
 * the given code is not available.
 */
int dpsoOcrGetLangIdx(const DpsoOcr* ocr, const char* langCode);


/**
 * Get whether the language is active.
 *
 * Returns false if langIdx is out of [0, dpsoOcrGetNumLangs()).
 */
bool dpsoOcrGetLangIsActive(const DpsoOcr* ocr, int langIdx);


/**
 * Set whether the language is active.
 *
 * Does nothing if langIdx is out of [0, dpsoOcrGetNumLangs()).
 */
void dpsoOcrSetLangIsActive(
    DpsoOcr* ocr, int langIdx, bool newIsActive);


/**
 * Get the number of active languages.
 */
int dpsoOcrGetNumActiveLangs(const DpsoOcr* ocr);


typedef enum {
    /**
     * Automatic text segmentation.
     *
     * Try to detect and split independent text blocks, like columns.
     */
    dpsoOcrJobTextSegmentation = 1 << 0
} DpsoOcrJobFlag;


/**
 * OCR job arguments.
 */
typedef struct DpsoOcrJobArgs {
    /**
     * The rectangle to capture from screen for recognition.
     */
    DpsoRect screenRect;

    /**
     * Combination of DpsoOcrJobFlag flags.
     */
    unsigned flags;
} DpsoOcrJobArgs;


/**
 * Queue OCR job.
 *
 * The function captures an image DpsoOcrJobArgs::screenRect from the
 * screen and queues it for OCR with the currently active languages.
 * On failure, sets an error message (dpsoGetError()) and returns
 * false. Reasons include:
 *   * jobArgs is null
 *   * jobArgs::screenRect is empty or outside the screen bounds
 *   * No active languages
 *   * A language manager for the same engine and data dir is active
 *   * Error when taking a screenshot
 */
bool dpsoOcrQueueJob(DpsoOcr* ocr, const DpsoOcrJobArgs* jobArgs);


typedef struct DpsoOcrProgress {
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
     * jobs are completed, or no jobs was queued after
     * dpsoOcrCreate().
     */
    int totalJobs;
} DpsoOcrProgress;


/**
 * Check if two DpsoOcrProgress are equal.
 */
bool dpsoOcrProgressEqual(
    const DpsoOcrProgress* a, const DpsoOcrProgress* b);


/**
 * Get jobs progress.
 *
 * If you just need to test if there are pending jobs, consider using
 * dpsoOcrHasPendingJobs().
 */
void dpsoOcrGetProgress(
    const DpsoOcr* ocr, DpsoOcrProgress* progress);


/**
 * Check if there are pending jobs.
 *
 * This is basically the same as testing DpsoOcrProgress::totalJobs.
 */
bool dpsoOcrHasPendingJobs(const DpsoOcr* ocr);


/**
 * Result of a single OCR job.
 */
typedef struct DpsoOcrJobResult {
    /**
     * Null-terminated text in UTF-8 encoding.
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
} DpsoOcrJobResult;


/**
 * Reference to internal array containing OCR job results.
 *
 * The results remain valid till the next dpsoOcrFetchResults() or
 * dpsoOcrDelete() call.
 *
 * \sa dpsoOcrFetchResults
 */
typedef struct DpsoOcrJobResults {
    const DpsoOcrJobResult* items;
    int numItems;
} DpsoOcrJobResults;


/**
 * Fetch job results.
 *
 * The function returns a reference to an internal array filled with
 * results of completed OCR jobs; if there are no new completed jobs,
 * the array will be empty. The previously returned results are
 * invalidated.
 */
void dpsoOcrFetchResults(DpsoOcr* ocr, DpsoOcrJobResults* results);


/**
 * Terminate jobs.
 *
 * This function terminates the active job, clears the job queue, and
 * drops all unfetched job results. dpsoOcrTerminateJobs() is
 * implicitly called on dpsoOcrDelete().
 */
void dpsoOcrTerminateJobs(DpsoOcr* ocr);


#ifdef __cplusplus
}


#include <memory>


namespace dpso {


struct OcrDeleter {
    void operator()(DpsoOcr* ocr) const
    {
        dpsoOcrDelete(ocr);
    }
};


using OcrUPtr = std::unique_ptr<DpsoOcr, OcrDeleter>;


}


#endif
