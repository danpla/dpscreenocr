#include "sound.h"

#include <string>

#include "dpso_sound/sound.h"

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"

#include "app_dirs.h"


using namespace dpso;


namespace {


struct CacheEntry {
    UiSoundId soundId;
    const char* soundName;
    std::string customFilePath{};
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


std::string getSoundPath(const CacheEntry& cacheEntry)
{
    if (!cacheEntry.customFilePath.empty())
        return cacheEntry.customFilePath;

    return
        std::string{uiGetAppDir(UiAppDirData)}
        + *os::dirSeparators
        + "sounds"
        + *os::dirSeparators
        + cacheEntry.soundName
        + ".wav";
}


bool initPlayer(CacheEntry& cacheEntry)
{
    const auto soundPath = getSoundPath(cacheEntry);
    try {
        cacheEntry.player = sound::Player::create(soundPath.c_str());
    } catch (sound::Error& e) {
        setError(
            "sound::Player::create(\"{}\"): {}", soundPath, e.what());
        return false;
    }

    return true;
}


}


bool uiSoundIsAvailable(void)
{
    return sound::isAvailable();
}


bool uiSoundSetFilePath(UiSoundId soundId, const char* filePath)
{
    auto* cacheEntry = findCacheEntry(soundId);
    if (!cacheEntry) {
        setError("Invalid soundId");
        return false;
    }

    cacheEntry->customFilePath = filePath;
    cacheEntry->player.reset();
    return initPlayer(*cacheEntry);
}


bool uiSoundPlay(UiSoundId soundId)
{
    auto* cacheEntry = findCacheEntry(soundId);
    if (!cacheEntry) {
        setError("Invalid soundId");
        return false;
    }

    if (!cacheEntry->player && !initPlayer(*cacheEntry))
        return false;

    try {
        cacheEntry->player->play();
    } catch (sound::Error& e) {
        setError(
            "sound::Player::play() ({}): {}",
            getSoundPath(*cacheEntry), e.what());
        return false;
    }

    return true;
}
