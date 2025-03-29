#include "sound.h"

#include <windows.h>

#include "dpso_utils/os.h"
#include "dpso_utils/str.h"

#include "error.h"


namespace dpso::sound {
namespace {


class WindowsPlayer : public Player {
public:
    explicit WindowsPlayer(const char* filePath)
    {
        try {
            wavData = os::loadData(filePath);
        } catch (os::Error& e) {
            throw Error{str::format("os::loadData(): {}", e.what())};
        }
    }

    ~WindowsPlayer()
    {
        // With SND_ASYNC and SND_MEMORY flags, the sound data must
        // remain valid while plying, so stop before releasing it.
        PlaySoundW(nullptr, nullptr, 0);
    }

    void play() override
    {
        if (!PlaySoundW(
                reinterpret_cast<LPCWSTR>(wavData.data()),
                nullptr,
                SND_ASYNC | SND_MEMORY | SND_NODEFAULT))
            throw Error{"PlaySoundW() failed"};
    }
private:
    std::string wavData;
};


}


bool isAvailable()
{
    return true;
}


const char* getSystemSoundsDirPath()
{
    return "C:\\Windows\\Media";
}


const std::vector<FormatInfo>& getSupportedFormats()
{
    // PlaySoundW() only supports WAV.
    static const std::vector<FormatInfo> result{{"WAV", {".wav"}}};
    return result;
}


std::unique_ptr<Player> Player::create(const char* filePath)
{
    return std::make_unique<WindowsPlayer>(filePath);
}


}
