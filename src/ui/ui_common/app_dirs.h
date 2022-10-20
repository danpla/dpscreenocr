
#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize app directory paths.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool uiInitAppDirs(const char* argv0);


typedef enum {
    /**
     * General data directory.
     */
    UiAppDirData,

    /**
     * Directory with documents like the user manual, license, etc.
     */
    UiAppDirDoc,

    /**
     * Localization data for bindtextdomain().
     */
    UiAppDirLocale
} UiAppDir;


/**
 * Get app directory path.
 *
 * The returned string is valid till the next call to uiGetDir().
 */
const char* uiGetAppDir(UiAppDir dir);


#ifdef __cplusplus
}
#endif
