
/**
 * \file
 * Access to platform-specific config directory
 */

#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get configuration directory.
 *
 * The function returns a platform-specific path where config files
 * can be saved. appName becomes a subdirectory of the path. The
 * directory chain is created if not already exists.
 *
 * On failure, and sets an error message (dpsoGetError()) and returns
 * null.
 */
const char* dpsoGetCfgPath(const char* appName);


#ifdef __cplusplus
}
#endif
