#pragma once

#include "format_info.h"
#include "unix/audio_data.h"


namespace dpso::sound::sndfile {


// Implementation of sound::getSystemSoundsDirPath(). Doesn't throw.
std::vector<FormatInfo> getSupportedFormats();


// Throws sound::Error.
AudioData loadAudioData(const char* filePath);


}
