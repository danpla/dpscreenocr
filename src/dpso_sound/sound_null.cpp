#include "sound.h"


namespace dpso::sound {


bool isAvailable()
{
    return false;
}


const char* getSystemSoundsDirPath()
{
    return "";
}


const std::vector<FormatInfo>& getSupportedFormats()
{
    static const std::vector<FormatInfo> result;
    return result;
}


std::unique_ptr<Player> Player::create(const char* filePath)
{
    (void)filePath;

    throw Error{"Sound library is not available"};
}


}
