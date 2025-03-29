#pragma once

#include <memory>
#include <vector>

#include "format_info.h"


namespace dpso::sound {


bool isAvailable();


// Return a path to a directory containing system sound files, or an
// empty string if no such directory is defined for the current
// platform. The function only returns a string and does not query the
// file system to check if the directory actually exists.
const char* getSystemSoundsDirPath();


// Return the list of audio file formats supported by Player. The list
// will be empty if isAvailable() is false or on error.
const std::vector<FormatInfo>& getSupportedFormats();


// The Player class is intended to play short notification sounds.
class Player {
public:
    // Throws sound:::Error.
    static std::unique_ptr<Player> create(const char* filePath);

    virtual ~Player() = default;

    // play() will stop a currently playing sound, even if it was
    // started by another Player instance. Throws sound:::Error.
    virtual void play() = 0;
};


}
