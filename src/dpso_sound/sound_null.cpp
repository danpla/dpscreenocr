#include "sound.h"


namespace dpso::sound {


bool isAvailable()
{
    return false;
}


std::unique_ptr<Player> Player::create(const char* filePath)
{
    (void)filePath;

    throw Error{"Sound library is not available"};
}


}
