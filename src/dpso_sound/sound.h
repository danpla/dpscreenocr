
#pragma once

#include <memory>
#include <stdexcept>


namespace dpso::sound {


class Error : public std::runtime_error {
    using runtime_error::runtime_error;
};


bool isAvailable();


class Player {
public:
    static std::unique_ptr<Player> create(
        const char* appName, const char* filePath);

    virtual ~Player() = default;

    virtual void play() = 0;
};


}
