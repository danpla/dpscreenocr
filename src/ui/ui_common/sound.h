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
 * Play a sound.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool uiSoundPlay(UiSoundId soundId);


#ifdef __cplusplus
}
#endif
