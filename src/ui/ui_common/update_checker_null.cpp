
#include "update_checker.h"

#include "dpso_utils/error_set.h"


bool uiUpdateCheckerIsAvailable(void)
{
    return false;
}


const char* uiUpdateCheckerGetPlatformId(void)
{
    return "";
}


struct UiUpdateChecker {
};


UiUpdateChecker* uiUpdateCheckerCreate(
    const char* appVersion,
    const char* userAgent,
    const char* infoFileUrl)
{
    (void)appVersion;
    (void)userAgent;
    (void)infoFileUrl;

    return new UiUpdateChecker{};
}


void uiUpdateCheckerDelete(UiUpdateChecker* updateChecker)
{
    delete updateChecker;
}


void uiUpdateCheckerStartCheck(UiUpdateChecker* updateChecker)
{
    (void)updateChecker;
}


bool uiUpdateCheckerIsCheckInProgress(
    const UiUpdateChecker* updateChecker)
{
    (void)updateChecker;
    return false;
}


UiUpdateCheckerStatus uiUpdateCheckerGetUpdateInfo(
    UiUpdateChecker* updateChecker,
    UiUpdateCheckerUpdateInfo* updateInfo)
{
    (void)updateChecker;
    (void)updateInfo;

    dpso::setError("Update checker was disabled at compile time");
    return UiUpdateCheckerStatusGenericError;
}
