
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get URL of the JSON info file for uiUpdateCheckerCreate().
 *
 * Returns "{uiAppWebsite}/version_info/{platform_id}.json", where
 * "platform_id" is the ID from uiUpdateCheckerGetPlatformId().
 * Returns an empty string if the update checker is not available.
 */
const char* uiUpdateCheckerGetInfoFileUrl(void);


#ifdef __cplusplus
}
#endif
