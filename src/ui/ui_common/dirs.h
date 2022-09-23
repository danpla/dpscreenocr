
#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize directory paths.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool uiInitDirs(const char* argv0);


typedef enum {
    /**
     * General data directory.
     */
    UiDirData,

    /**
     * Directory with documents like the user manual, license, etc.
     */
    UiDirDoc,

    /**
     * Localization data for bindtextdomain().
     */
    UiDirLocale
} UiDir;


/**
 * Get directory path.
 *
 * The returned string is valid till the next call to uiGetDir().
 */
const char* uiGetDir(UiDir dir);


#ifdef __cplusplus
}
#endif
