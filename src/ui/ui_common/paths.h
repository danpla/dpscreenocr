
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize base directory path.
 *
 * On failure, sets an error message (dpsoGetError()) and returns 0.
 */
int uiInitBaseDirPath(const char* argv0);


/**
 * Get the base path for all other subdirectories.
 *
 * All ui*Dir constants are intended to be appended to this path.
 *
 * The prefix depends on the platform. For example:
 *
 * * On Windows, this is the directory of the executable.
 * * On Unix-like systems, this is the parent directory of the
 *   executable (it's assumed that the executable is in bin/), or the
 *   absolute installation prefix if relocation is not supported or
 *   was disabled at compile time.
 */
const char* uiGetBaseDirPath(void);


/**
 * General data directory.
 */
extern const char* const uiDataDir;


/**
 * Path to directory with documents.
 */
extern const char* const uiDocDir;


/**
 * Path to localization data for bindtextdomain().
 */
extern const char* const uiLocaleDir;


#ifdef __cplusplus
}
#endif
