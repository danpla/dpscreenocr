#pragma once

#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * OCR language manager.
 *
 * The language manager allows you to install, update, and remove
 * languages.
 *
 * DpsoOcrLangManager and all DpsoOcr instances created for the same
 * engine and data dir are synchronized as follows. When you create
 * DpsoOcrLangManager, all DpsoOcr instances will finish their jobs as
 * if by calling dpsoOcrWaitJobsToComplete(). Until DpsoOcrLangManager
 * is deleted, DpsoOcr instances will not allow queuing new jobs, and
 * their language lists will be frozen (that is, they will not reflect
 * changes made via the language manager).
 */
typedef struct DpsoOcrLangManager DpsoOcrLangManager;


/**
 * Create language manager.
 *
 * engineIdx and dataDir arguments are the same as for the DpsoOcr
 * object.
 *
 * userAgent sets the user agent for HTTP connections. Most servers
 * will refuse to negotiate with a client that don't provide a user
 * agent string. In its simplest form, this string consists of the
 * application name, optionally followed by a forward slash and a
 * version number, like "AppName/1.0.0". See the description of the
 * "User-Agent" from RFC 1945 for the details.
 *
 * infoFileUrl is a HTTP(S) URL of a JSON file containing information
 * about external languages available for downloading. The root of the
 * JSON is an array of objects containing the following keys:
 *
 * * code - a string containing the language code.
 *
 * * sha256 - a string containing the SHA-256 checksum of the file.
 *
 * * size - a number indicating the size of the file, in bytes.
 *
 * * url - a string containing the URL of the file.
 *
 * userAgent and infoFileUrl are ignored by engines that don't use the
 * network connection.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * null. In particular, the function will fail if another language
 * manager for the same engine and data dir is active.
 */
DpsoOcrLangManager* dpsoOcrLangManagerCreate(
    int engineIdx,
    const char* dataDir,
    const char* userAgent,
    const char* infoFileUrl);


/**
 * Delete language manager.
 *
 * The function implicitly calls dpsoOcrLangManagerCancelInstall().
 */
void dpsoOcrLangManagerDelete(DpsoOcrLangManager* langManager);


/**
 * Get number of available languages.
 *
 * The language list is sorted by language code. You can use
 * dpsoOcrLangManagerGetLangIdx() to get the language index using
 * binary search.
 */
int dpsoOcrLangManagerGetNumLangs(
    const DpsoOcrLangManager* langManager);


/**
 * Get language index.
 *
 * The function uses binary search. Returns -1 if the language with
 * the given code is not available.
 */
int dpsoOcrLangManagerGetLangIdx(
    const DpsoOcrLangManager* langManager, const char* langCode);


/**
 * Get language code.
 *
 * Returns an empty string if langIdx is out of
 * [0, dpsoOcrLangManagerGetNumLangs()).
 */
const char* dpsoOcrLangManagerGetLangCode(
    const DpsoOcrLangManager* langManager, int langIdx);


/**
 * Get language name.
 *
 * Returns an empty string if the language has no name, or if langIdx
 * is out of [0, dpsoOcrLangManagerGetNumLangs()).
 */
const char* dpsoOcrLangManagerGetLangName(
    const DpsoOcrLangManager* langManager, int langIdx);


typedef enum {
    DpsoOcrLangStateNotInstalled,

    /**
     * Language is installed.
     *
     * If there's an update available for the given language, the
     * DpsoOcrLangStateUpdateAvailable state is used instead.
     */
    DpsoOcrLangStateInstalled,

    /**
     * An update is available for the language.
     *
     * This state implies that the language is installed
     * (DpsoOcrLangStateInstalled).
     */
    DpsoOcrLangStateUpdateAvailable
} DpsoOcrLangState;


DpsoOcrLangState dpsoOcrLangManagerGetLangState(
    const DpsoOcrLangManager* langManager, int langIdx);


/**
 * A language size in bytes.
 */
typedef struct DpsoOcrLangSize {
    /**
     * The size of an external language.
     *
     * -1 if the external size is unknown, i.e. either no external
     * languages were loaded or a manually installed language was not
     * in the list of external languages.
     */
    int64_t external;

    /**
     * The size of an installed language.
     *
     * -1 if the language is not installed.
     */
    int64_t local;
} DpsoOcrLangSize;


void dpsoOcrLangManagerGetLangSize(
    const DpsoOcrLangManager* langManager,
    int langIdx,
    DpsoOcrLangSize* langSize);


typedef enum {
    /**
     * Operation was either not started or was interrupted.
     */
    DpsoOcrLangOpStatusCodeNone,

    /**
     * Operation is in progress.
     */
    DpsoOcrLangOpStatusCodeProgress,

    /**
     * Operation finished successfully.
     */
    DpsoOcrLangOpStatusCodeSuccess,

    /**
     * There was an error.
     *
     * This enumerator represents an error that doesn't fall into any
     * category described by the remaining error status codes. If you
     * are not interested in a specific error category and just want
     * to test for errors in general, check if the status code >= this
     * enumerator, or use dpsoOcrLangOpStatusIsError() that does the
     * same.
     */
    DpsoOcrLangOpStatusCodeGenericError,

    /**
     * Connection could not be established or was terminated.
     */
    DpsoOcrLangOpStatusCodeNetworkConnectionError,
} DpsoOcrLangOpStatusCode;


/**
 * Returns true if code >= DpsoOcrLangOpStatusCodeGenericError.
 */
bool dpsoOcrLangOpStatusIsError(DpsoOcrLangOpStatusCode code);


typedef struct DpsoOcrLangOpStatus {
    DpsoOcrLangOpStatusCode code;

    /**
     * Error text.
     *
     * The text is empty if dpsoOcrLangOpStatusIsError() is false.
     * The pointer remains valid till the next call to the function
     * that returned this DpsoOcrLangOpStatus.
     */
    const char* errorText;
} DpsoOcrLangOpStatus;


/**
 * Start fetching the list of external languages.
 *
 * The function starts fetching the list of languages available for
 * installation from an external source, like a HTTP server or a
 * package manager. Once the list is successfully fetched, you can use
 * dpsoOcrLangManagerLoadFetchedExternalLangs() to merge it with the
 * list of existing (already installed) languages.
 *
 * If the function succeeds, all previously loaded external languages
 * (see dpsoOcrLangManagerLoadFetchedExternalLangs()) will be removed.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false. In particular, the function will fail when called while
 * fetching or installation is active.
 */
bool dpsoOcrLangManagerStartFetchExternalLangs(
    DpsoOcrLangManager* langManager);


void dpsoOcrLangManagerGetFetchExternalLangsStatus(
    const DpsoOcrLangManager* langManager,
    DpsoOcrLangOpStatus* status);


/**
 * Load the fetched list of external languages.
 *
 * If fetching is currently active, the function will block till it
 * finishes.
 *
 * If an external language is not installed, it is added to the list
 * with the DpsoOcrLangStateNotInstalled state. If it's a newer
 * version of an installed language, the state of that language will
 * change from DpsoOcrLangStateInstalled to
 * DpsoOcrLangStateUpdateAvailable.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false. In particular, this will happen if fetching either was not
 * started or failed.
 */
bool dpsoOcrLangManagerLoadFetchedExternalLangs(
    DpsoOcrLangManager* langManager);


/**
 * Get whether the language is marked for installation.
 *
 * Returns false if langIdx is out of bounds.
 */
bool dpsoOcrLangManagerGetInstallMark(
    const DpsoOcrLangManager* langManager, int langIdx);


/**
 * Mark/unmark a language for installation.
 *
 * The function will do nothing if installation is active or if the
 * language has the DpsoOcrLangStateInstalled state. Similarly, if the
 * language state turns to DpsoOcrLangStateInstalled (for example,
 * as a result of dpsoOcrLangManagerStartFetchExternalLangs()), its
 * installation mark is removed.
 */
void dpsoOcrLangManagerSetInstallMark(
    DpsoOcrLangManager* langManager, int langIdx, bool installMark);


/**
 * Start language installation.
 *
 * The function starts installation of languages currently marked with
 * dpsoOcrLangManagerSetInstallMark(). If the function succeeds, the
 * installation marks are cleared; if it fails, they are retained.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false. The reasons include:
 *   * langManager is null
 *   * Installation is active
 *   * No languages were marked for installation
 */
bool dpsoOcrLangManagerStartInstall(DpsoOcrLangManager* langManager);


typedef struct DpsoOcrLangInstallProgress {
    /**
     * Index of the language being installed.
     *
     * This is the index expected by dpsoOcrLangManagerGetLang*() and
     * similar functions.
     *
     * -1 if installation is not active.
     */
    int curLangIdx;

    /**
     * Installation progress of the current language.
     *
     * The progress value is either a percentage (0-100), or -1 if the
     * final size of the language file is unknown.
     */
    int curLangProgress;

    /**
     * 1-based number of the language being installed.
     *
     * The number is > 0 and <= totalLangs, or 0 if installation is
     * not active.
     */
    int curLangNum;

    /**
     * Total number of languages.
     *
     * This is the total number of languages that were marked for
     * installation on dpsoOcrLangManagerStartInstallLang() call.
     *
     * 0 if installation is not active.
     */
    int totalLangs;
} DpsoOcrLangInstallProgress;


void dpsoOcrLangManagerGetInstallProgress(
    const DpsoOcrLangManager* langManager,
    DpsoOcrLangInstallProgress* progress);


void dpsoOcrLangManagerGetInstallStatus(
    const DpsoOcrLangManager* langManager,
    DpsoOcrLangOpStatus* status);


/**
 * Cancel installation.
 *
 * Already installed languages will not be removed.
 */
void dpsoOcrLangManagerCancelInstall(DpsoOcrLangManager* langManager);


/**
 * Remove language.
 *
 * If the removed language was in the list of external languages
 * fetched by dpsoOcrLangManagerFetchExternalLangs(), its state will
 * change to DpsoOcrLangStateNotInstalled. Otherwise, the language
 * is removed from the list.
 *
 * The function does nothing and returns true if the language state is
 * DpsoOcrLangStateNotInstalled.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false. Reasons include:
 *   * Language fetching is active.
 *   * Installation is active, even if the language you are trying to
 *     remove was not marked for installation.
 */
bool dpsoOcrLangManagerRemoveLang(
    DpsoOcrLangManager* langManager, int langIdx);


#ifdef __cplusplus
}


#include <memory>


namespace dpso {


struct OcrLangManagerDeleter {
    void operator()(DpsoOcrLangManager* langManager) const
    {
        dpsoOcrLangManagerDelete(langManager);
    }
};


using OcrLangManagerUPtr =
    std::unique_ptr<DpsoOcrLangManager, OcrLangManagerDeleter>;


}


#endif
