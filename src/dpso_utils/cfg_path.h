
/**
 * \file
 * Access to platform-specific config directory
 */

#pragma once

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get configuration directory.
 *
 * The function returns a platform-specific path where config files
 * can be saved. appName becomes a subdirectory of the path. The
 * directory chain is created if not already exists.
 */
const char* dpsoGetCfgPath(const char* appName);


/**
 * Open FILE* in config directory.
 *
 * The function opens fileName in the platform-specific config
 * directory. fileName thus should be a base name.
 */
FILE* dpsoCfgPathFopen(
    const char* appName, const char* fileName, const char* mode);


#ifdef __cplusplus
}
#endif
