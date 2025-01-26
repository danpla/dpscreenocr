#pragma once

#include <memory>
#include <stdexcept>


namespace dpso::sound {


class Error : public std::runtime_error {
    using runtime_error::runtime_error;
};


bool isAvailable();


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
