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


static void setError()
{
    dpso::setError("Update checker was disabled at compile time");
}


UiUpdateChecker* uiUpdateCheckerCreate(
    const char* /*appVersion*/,
    const char* /*userAgent*/,
    const char* /*infoFileUrl*/)
{
    setError();
    return nullptr;
}


void uiUpdateCheckerDelete(UiUpdateChecker* /*updateChecker*/)
{
}


void uiUpdateCheckerStartCheck(UiUpdateChecker* /*updateChecker*/)
{
}


bool uiUpdateCheckerIsCheckInProgress(
    const UiUpdateChecker* /*updateChecker*/)
{
    return false;
}


UiUpdateCheckerStatus uiUpdateCheckerGetUpdateInfo(
    UiUpdateChecker* /*updateChecker*/,
    UiUpdateCheckerUpdateInfo* /*updateInfo*/)
{
    setError();
    return UiUpdateCheckerStatusGenericError;
}
