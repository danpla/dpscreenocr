
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


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
