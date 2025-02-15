#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "dpso_img/img.h"


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


int dpsoOcrGetNumLangs(const DpsoOcr* ocr);


/**
 * Get language code.
 *
 * A language code is a string that uniquely identify a language,
 * like an ISO 639 code. It contains only ASCII alphanumeric
 * characters, hyphens, underscores, and periods.
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


int dpsoOcrGetNumActiveLangs(const DpsoOcr* ocr);


typedef enum {
    /**
     * Automatic text segmentation.
     *
     * Try to detect and split independent text blocks, like columns.
     */
    dpsoOcrJobTextSegmentation = 1 << 0
} DpsoOcrJobFlag;


typedef unsigned DpsoOcrJobFlags;


/**
 * Queue an OCR job.
 *
 * The function queues an image for OCR with the currently active
 * languages. dpsoOcrQueueJob() takes ownership of the image and sets
 * *img to null, even on failure.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false. Reasons include:
 *   * img or *img is null
 *   * No active languages
 *   * A language manager for the same engine and data dir is active
 */
bool dpsoOcrQueueJob(
    DpsoOcr* ocr, DpsoImg** img, DpsoOcrJobFlags flags);


typedef struct DpsoOcrProgress {
    /**
     * Progress of the current job in percents.
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
     * Can be zero if there are no pending jobs, meaning that either
     * all jobs are completed or no jobs were queued after
     * dpsoOcrCreate().
     */
    int totalJobs;
} DpsoOcrProgress;


bool dpsoOcrProgressEqual(
    const DpsoOcrProgress* a, const DpsoOcrProgress* b);


void dpsoOcrGetProgress(
    const DpsoOcr* ocr, DpsoOcrProgress* progress);


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
     * Timestamp in the "YYYY-MM-DD hh:mm:ss" format.
     */
    const char* timestamp;
} DpsoOcrJobResult;


/**
 * Check for pending results.
 *
 * Returns true if at least one result of the job is currently
 * available or will be available in the future after the job
 * completes. Returns false if no jobs have been queued or if you have
 * already received the result of the last queued job.
 */
bool dpsoOcrHasPendingResults(const DpsoOcr* ocr);


/**
 * Get the result of the next completed OCR job.
 *
 * Returns true if the result is returned. The result remains valid
 * till the next call to dpsoOcrGetResult(), dpsoOcrTerminateJobs(),
 * or dpsoOcrDelete(). The previously returned result is invalidated.
 */
bool dpsoOcrGetResult(DpsoOcr* ocr, DpsoOcrJobResult* result);


/**
 * Terminate jobs.
 *
 * This function terminates the active job, clears the job queue, and
 * drops all pending job results. dpsoOcrTerminateJobs() is
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
