#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get a string suitable for the User-Agent HTTP header.
 *
 * The string is built from uiAppName and uiAppVersion.
 */
const char* uiGetUserAgent(void);


#ifdef __cplusplus
}
#endif
