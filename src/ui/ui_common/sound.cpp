
#include "sound.h"

#include <cassert>
#include <string>

#include "dpso_sound/sound.h"

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"

#include "app_dirs.h"
#include "app_info.h"


using namespace dpso;


namespace {


struct CacheEntry {
    UiSoundId soundId;
    const char* soundName;
    std::unique_ptr<sound::Player> player{};
} cache[]{
    {UiSoundIdDone, "done"},
};


CacheEntry* findCacheEntry(UiSoundId soundId)
{
    for (auto& entry : cache)
        if (entry.soundId == soundId)
            return &entry;

    return {};
}


std::string getSoundPath(const std::string& soundName)
{
    return
        std::string{uiGetAppDir(UiAppDirData)}
        + *os::dirSeparators
        + "sounds"
        + *os::dirSeparators
        + soundName
        + ".wav";
}


}


bool uiSoundIsAvailable(void)
{
    return sound::isAvailable();
}


bool uiSoundPlay(UiSoundId soundId)
{
    auto* cacheEntry = findCacheEntry(soundId);
    if (!cacheEntry) {
        setError("Invalid soundId");
        return false;
    }

    if (!cacheEntry->player) {
        const auto soundPath = getSoundPath(cache->soundName);

        try {
            cacheEntry->player = sound::Player::create(
                uiAppName, soundPath.c_str());
        } catch (sound::Error& e) {
            setError(
                "sound::Player::create(\"{}\", \"{}\"): {}",
                uiAppName, soundPath, e.what());
            return false;
        }
    }

    try {
        cacheEntry->player->play();
    } catch (sound::Error& e) {
        setError(
            "sound::Player::play() ({}): {}",
            cacheEntry->soundName, e.what());
        return false;
    }

    return true;
}
