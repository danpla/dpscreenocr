
/**
 * \file
 * OCR routines
 */

#pragma once

#include <stddef.h>

#include "geometry_c.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get the number of available OCR engines.
 */
int dpsoOcrGetNumEngines(void);


/**
 * What DpsoOcrArgs::dataDir the OCR engine prefers.
 */
typedef enum {
    /**
     * Engine doesn't use external data.
     *
     * DpsoOcrArgs::dataDir is ignored.
     */
    DpsoOcrEngineDataDirPreferenceNoDataDir,

    /**
     * Engine prefers the default data directory.
     *
     * Usually this means that on the current platform, the data is
     * installed in a system-wide path that is hardcoded into the OCR
     * library at build time.
     */
    DpsoOcrEngineDataDirPreferencePreferDefault,

    /**
     * Engine prefers an explicit path to data directory.
     *
     * This is normally the default mode for non-Unix platforms, where
     * the data is usually installed in the application-specific
     * directory.
     */
    DpsoOcrEngineDataDirPreferencePreferExplicit
} DpsoOcrEngineDataDirPreference;


struct DpsoOcrEngineInfo {
    /**
     * Unique engine id.
     *
     * An id consists of lower-case ASCII alphabetical letters,
     * numbers, and uses underscore to separate words.
     */
    const char* id;

    /**
     * Engine name.
     *
     * This is normal, readable engine name, which, unlike id, doesn't
     * have any restrictions.
     */
    const char* name;

    DpsoOcrEngineDataDirPreference dataDirPreference;
};


/**
 * Get OCR engine info.
 *
 * Does nothing if the index is out of [0, dpsoOcrGetNumEngines()).
 */
void dpsoOcrGetEngineInfo(int idx, struct DpsoOcrEngineInfo* info);


struct DpsoOcrArgs {
    const char* engineId;

    /**
     * Path to OCR engine data directory.
     *
     * May be null to use the default directory, which is mostly
     * useful on Unix-like systems where this path is hardcoded when
     * the OCR library is built. See DpsoOcrEngineDataDirPreference
     * for the details.
     */
    const char* dataDir;
};


struct DpsoOcr;


/**
 * Create OCR.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * null.
 */
struct DpsoOcr* dpsoOcrCreate(const struct DpsoOcrArgs* ocrArgs);


void dpsoOcrDelete(struct DpsoOcr* ocr);


/**
 * Get the number of available languages.
 */
int dpsoOcrGetNumLangs(const struct DpsoOcr* ocr);


/**
 * Get the language code.
 *
 * Returns an empty string if langIdx is out of
 * [0, dpsoOcrGetNumLangs()).
 */
const char* dpsoOcrGetLangCode(
    const struct DpsoOcr* ocr, int langIdx);


/**
 * Get default language code.
 *
 * The main purpose of the default language code is to select
 * a language when a program starts for the first time.
 *
 * Returns an empty string if the OCR engine has no meaningful default
 * language.
 */
const char* dpsoOcrGetDefaultLangCode(const struct DpsoOcr* ocr);


/**
 * Get language name.
 *
 * Returns null if the language name for the given code is not known.
 */
const char* dpsoOcrGetLangName(
    const struct DpsoOcr* ocr, const char* langCode);


/**
 * Get language index.
 *
 * Returns -1 if the language with the given code is not available.
 */
int dpsoOcrGetLangIdx(
    const struct DpsoOcr* ocr, const char* langCode);


/**
 * Get whether the language is active.
 *
 * Returns 0 if langIdx is out of [0, dpsoOcrGetNumLangs()).
 */
int dpsoOcrGetLangIsActive(const struct DpsoOcr* ocr, int langIdx);


/**
 * Set whether the language is active.
 *
 * Does nothing if langIdx is out of [0, dpsoOcrGetNumLangs()).
 */
void dpsoOcrSetLangIsActive(
    struct DpsoOcr* ocr, int langIdx, int newIsActive);


/**
 * Get the number of active languages.
 */
int dpsoOcrGetNumActiveLangs(const struct DpsoOcr* ocr);


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
struct DpsoOcrJobArgs {
    /**
     * The rectangle to capture from screen for recognition.
     */
    struct DpsoRect screenRect;

    /**
     * Combination of DpsoOcrJobFlag flags.
     */
    unsigned flags;
};


/**
 * Queue OCR job.
 *
 * The function captures an image DpsoOcrJobArgs::screenRect from the
 * screen and queues it for OCR with the currently active languages.
 * On failure, sets an error message (dpsoGetError()) and returns 0.
 * Reasons include:
 *   * jobArgs is null
 *   * jobArgs::screenRect is empty or outside the screen bounds
 *   * No active languages
 *   * Error when taking a screenshot
 *
 * Tesseract versions before 4.1.0 only work with "C" locale. This
 * locale is automatically set on a successful dpsoOcrQueueJob() call,
 * and restored after dpsoOcrWaitJobsToComplete(),
 * dpsoOcrTerminateJobs(), or dpsoOcrFetchResults() when all jobs are
 * completed. Don't change the locale between these two points.
 */
int dpsoOcrQueueJob(
    struct DpsoOcr* ocr, const struct DpsoOcrJobArgs* jobArgs);


struct DpsoOcrProgress {
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
};


/**
 * Return 1 if two DpsoOcrProgress are equal, 0 otherwise.
 */
int dpsoOcrProgressEqual(
    const struct DpsoOcrProgress* a, const struct DpsoOcrProgress* b);


/**
 * Get jobs progress.
 *
 * If you just need to test if there are pending jobs, consider using
 * dpsoOcrGetJobsPending().
 */
void dpsoOcrGetProgress(
    const struct DpsoOcr* ocr, struct DpsoOcrProgress* progress);


/**
 * Check if there are pending jobs.
 *
 * The function returns 1 if there are pending jobs (that is, there
 * is an active or at least one queued job), 0 otherwise. This is
 * basically the same as testing DpsoOcrProgress::totalJobs.
 */
int dpsoOcrGetJobsPending(const struct DpsoOcr* ocr);


/**
 * Result of a single OCR job.
 */
struct DpsoOcrJobResult {
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
 * The results remain valid till the next dpsoOcrFetchResults() or
 * dpsoOcrDelete() call.
 *
 * \sa dpsoOcrFetchResults
 */
struct DpsoOcrJobResults {
    const struct DpsoOcrJobResult* items;
    int numItems;
};


/**
 * Fetch job results.
 *
 * The function returns a reference to an internal array filled with
 * results of completed OCR jobs; if there are no new completed jobs,
 * the array will be empty. The previously returned results are
 * invalidated.
 */
void dpsoOcrFetchResults(
    struct DpsoOcr* ocr, struct DpsoOcrJobResults* results);


/**
 * Progress callback for dpsoOcrWaitJobsToComplete().
 *
 * You can use dpsoOcrGetProgress() inside the callback to get the
 * actual progress, and dpsoOcrTerminateJobs() to terminate jobs
 * prematurely.
 */
typedef void (*DpsoOcrProgressCallback)(void* userData);


/**
 * Block the current thread till all jobs are completed.
 *
 * The progress callback (may be null) is called every time the
 * progress changes.
 */
void dpsoOcrWaitJobsToComplete(
    struct DpsoOcr* ocr,
    DpsoOcrProgressCallback progressCallback,
    void* userData);


/**
 * Terminate jobs.
 *
 * This function terminates the active job, clears the job queue, and
 * drops all unfetched job results. dpsoOcrTerminateJobs() is
 * implicitly called on dpsoOcrDelete().
 */
void dpsoOcrTerminateJobs(struct DpsoOcr* ocr);


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
