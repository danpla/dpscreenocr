#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


bool uiSoundIsAvailable(void);


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
