
#pragma once

#include <stdbool.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Check if the update checker is available.
 *
 * The function only returns false if the update checker was disabled
 * at compile time. In this case, uiUpdateCheckerCreate() will return
 * a dummy checker that returns an error for
 * uiUpdateCheckerGetUpdateInfo().
 */
bool uiUpdateCheckerIsAvailable(void);


/**
 * Return a platform ID.
 *
 * You can use the ID as part of infoFileUrl for
 * uiUpdateCheckerCreate(). Possible values are:
 *
 *   * "linux"
 *   * "windows"
 *   * "generic" - returned on platforms not listed in the above IDs.
 *   * "" - returned if the update checker was disabled at compile
 *     time.
 */
const char* uiUpdateCheckerGetPlatformId(void);


typedef struct UiUpdateChecker UiUpdateChecker;


/**
 * Create update checker.
 *
 * appVersion is the current version of the application.
 *
 * userAgent is the user agent for HTTPS connections. See the
 * description of the "User-Agent" from RFC 1945 for the details.
 *
 * infoFileUrl is an HTTPS URL of a JSON file containing information
 * about available new versions. The root of the JSON is an array of
 * arbitrary ordered objects (usually at least one) containing the
 * following keys:
 *
 * * version - a version number string.
 *
 * * requirements - an object with information about minimum system
 *   requirements. The contents and structure of the object depends on
 *   the platform:
 *
 *   * For the "linux" platform ID, it contains a "glibc" key that
 *     maps to a glibc version string. See:
 *
 *     https://sourceware.org/glibc/wiki/Glibc%20Timeline
 *
 *   * For the "windows" platform ID, it contains a "windows-version"
 *     key that maps to a Windows version string in the
 *     "major.minor.build" format. See:
 *
 *     https://learn.microsoft.com/en-us/windows/win32/sysinfo/operating-system-version
 *     https://en.wikipedia.org/wiki/List_of_Microsoft_Windows_versions
 *
 *   * For the "generic" platform ID, the "requirements" object is
 *     ignored and can be empty.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * null.
 */
UiUpdateChecker* uiUpdateCheckerCreate(
    const char* appVersion,
    const char* userAgent,
    const char* infoFileUrl);


void uiUpdateCheckerDelete(UiUpdateChecker* updateChecker);


/**
 * Start an update check.
 *
 * Does nothing if the check is already in progress.
 */
void uiUpdateCheckerStartCheck(UiUpdateChecker* updateChecker);


bool uiUpdateCheckerIsCheckInProgress(
    const UiUpdateChecker* updateChecker);


typedef enum {
    /**
     * Update check finished successfully.
     */
    UiUpdateCheckerStatusSuccess,

    /**
     * There was an error.
     *
     * This enumerator represents an error that doesn't fall into any
     * category described by the remaining error status codes.
     */
    UiUpdateCheckerStatusGenericError,

    /**
     * Connection could not be established or was terminated.
     */
    UiUpdateCheckerStatusNetworkConnectionError,
} UiUpdateCheckerStatus;


/**
 * Unmet requirement of UiUpdateCheckerUpdateInfo::latestVersion.
 */
typedef struct UiUpdateCheckerUnmetRequirement {
    /**
     * Name of the requirement.
     *
     * For example, this can be a library name and version.
     */
    const char* required;

    /**
     * Actual state of the requirement.
     *
     * For example, this could be the library from the "required"
     * field, but with a smaller version. By convention, an empty
     * string indicates a missing requirement.
     */
    const char* actual;
} UiUpdateCheckerUnmetRequirement;


/**
 * Version update info.
 *
 * All pointers remain valid till the next call to
 * uiUpdateCheckerGetUpdateInfo().
 */
typedef struct UiUpdateCheckerUpdateInfo {
    /**
     * New version.
     *
     * This is the version that is greater than appVersion passed to
     * uiUpdateCheckerCreate() and meets the minimum requirements.
     * newVersion is empty if there is no newer version, or none of
     * the newer versions meet the minimum requirements.
     */
    const char* newVersion;

    /**
     * Latest available version.
     *
     * This is the latest version that is greater than appVersion
     * passed to uiUpdateCheckerCreate() that don't necessarily meet
     * the minimum requirements.
     *
     * If the latest version meets the minimum requirements,
     * latestVersion and newVersion will be the same, and the
     * unmetRequirements array will be empty. If the requirements are
     * not met, unmetRequirements will contain at least one item.
     */
    const char* latestVersion;

    /**
     * List of unmet requirements for latestVersion.
     */
    const UiUpdateCheckerUnmetRequirement* unmetRequirements;
    size_t numUnmetRequirements;
} UiUpdateCheckerUpdateInfo;


/**
 * Get the version update info.
 *
 * The function returns the result of the last check started with
 * uiUpdateCheckerStartCheck(). If the check is currently active, the
 * function will block until it is finished.
 *
 * On failure, sets an error message (dpsoGetError()), returns a
 * status other than UiUpdateCheckerStatusSuccess, and leaves
 * updateInfo unchanged.
 */
UiUpdateCheckerStatus uiUpdateCheckerGetUpdateInfo(
    UiUpdateChecker* updateChecker,
    UiUpdateCheckerUpdateInfo* updateInfo);


#ifdef __cplusplus
}


#include <memory>


namespace ui {


struct UpdateCheckerDeleter {
    void operator()(UiUpdateChecker* updateChecker) const
    {
        uiUpdateCheckerDelete(updateChecker);
    }
};


using UpdateCheckerUPtr =
    std::unique_ptr<UiUpdateChecker, UpdateCheckerDeleter>;


}


#endif
