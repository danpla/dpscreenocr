#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


bool uiSoundIsAvailable(void);


/**
 * Return a path to a directory containing system sound files.
 *
 * Returns an empty path if no such directory is defined for the
 * current platform. The function only returns a string and does not
 * query the file system to check if the directory actually exists.
 */
const char* uiSoundGetSystemSoundsDirPath(void);


typedef struct UiSoundFormatInfo {
    const char* name;

    /**
     * The list of possible file extensions for the format.
     *
     * Each extension has a leading period.
     */
    const char* const* extensions;
    int numExtensions;
} UiSoundFormatInfo;


/**
 * Return the number of supported audio file formats.
 */
int uiSoundGetNumFormats(void);


/**
 * Return the audio file format info.
 *
 * idx is the index in the range [0, uiSoundGetNumFormats()).
 */
void uiSoundGetFormatInfo(int idx, UiSoundFormatInfo* info);


typedef enum {
    UiSoundIdDone
} UiSoundId;


/**
 * Set a custom file path for a sound.
 *
 * An empty filePath resets to the default sound. On failure, sets an
 * error message (dpsoGetError()) and returns false.
 */
bool uiSoundSetFilePath(UiSoundId soundId, const char* filePath);


/**
 * Play a sound.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool uiSoundPlay(UiSoundId soundId);


#ifdef __cplusplus
}
#endif
