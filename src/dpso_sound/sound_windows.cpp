
#include "sound.h"

#include <cstdint>
#include <optional>
#include <vector>

#include <windows.h>

#include "dpso_utils/os.h"
#include "dpso_utils/str.h"
#include "dpso_utils/stream/file_stream.h"
#include "dpso_utils/stream/utils.h"


namespace dpso::sound {
namespace {


std::vector<std::uint8_t> loadData(const char* filePath)
{
    std::int64_t fileSize;
    try {
        fileSize = os::getFileSize(filePath);
    } catch (os::Error& e) {
        throw Error{str::format("os::getFileSize(): {}", e.what())};
    }

    std::optional<FileStream> file;
    try {
        file.emplace(filePath, FileStream::Mode::read);
    } catch (os::Error& e) {
        throw Error{str::format(
            "FileStream(..., Mode::read): {}", e.what())};
    }

    std::vector<std::uint8_t> result;
    result.resize(fileSize);

    try {
        read(*file, result.data(), result.size());
    } catch (StreamError& e) {
        throw Error{str::format("read(file, ...): {}", e.what())};
    }

    return result;
}


class WindowsPlayer : public Player {
public:
    explicit WindowsPlayer(const char* filePath)
    {
        try {
            wavData = loadData(filePath);
        } catch (Error& e) {
            throw Error{str::format("Can't load data: {}", e.what())};
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
    std::vector<std::uint8_t> wavData;
};


}


bool isAvailable()
{
    return true;
}


std::unique_ptr<Player> Player::create(
    const char* appName, const char* filePath)
{
    (void)appName;

    return std::make_unique<WindowsPlayer>(filePath);
}


}
