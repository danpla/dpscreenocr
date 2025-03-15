#pragma once


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    DpsoUserDirConfig,
    DpsoUserDirData
} DpsoUserDir;


/**
 * Get a path to a user directory.
 *
 * appName becomes a subdirectory of the path. On failure, and sets an
 * error message (dpsoGetError()) and returns null.
 */
const char* dpsoGetUserDir(DpsoUserDir userDir, const char* appName);


#ifdef __cplusplus
}
#endif
