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
 * On failure, and sets an error message (dpsoGetError()) and returns
 * null.
 */
const char* dpsoGetUserDir(DpsoUserDir userDir);


#ifdef __cplusplus
}
#endif
