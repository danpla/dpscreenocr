#include "sound.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <functional>
#include <future>
#include <utility>
#include <vector>

#include "dpso_utils/str.h"

#include "error.h"
#include "unix/lib/pulse.h"
#include "unix/lib/sndfile.h"
#include "unix/sndfile.h"


namespace dpso::sound {
namespace {


class Context {
public:
    static std::shared_ptr<Context> get()
    {
        static std::weak_ptr<Context> weakPtr;

        if (auto ctx = weakPtr.lock())
            return ctx;

        auto ctx = std::shared_ptr<Context>{new Context{}};
        weakPtr = ctx;

        return ctx;
    }

    ~Context()
    {
        stop();
    }

    void play(const AudioData& audioData);

    void stop(const AudioData& audioData)
    {
        if (&audioData == activeAudioData)
            stop();
    }
private:
    const LibPulse& libPulse{LibPulse::get()};
    const AudioData* activeAudioData{};
    std::promise<void> stopPromise;
    std::future<void> playFuture;

    Context() = default;

    void stop();
    static void playTask(
        const LibPulse& libPulse,
        PulseSimpleUPtr pulseSimple,
        const AudioData& audioData,
        std::future<void> stopFuture);
};


void Context::play(const AudioData& audioData)
{
    stop();

    static char appName[128];
    if (!*appName
            && !libPulse.get_binary_name(appName, sizeof(appName)))
        std::strncpy(appName, "dpso_sound", sizeof(appName) - 1);

    const LibPulse::sample_spec sampleSpec{
        LibPulse::SAMPLE_S16NE,
        static_cast<std::uint32_t>(audioData.rate),
        static_cast<std::uint8_t>(audioData.numChannels)};

    int error;
    PulseSimpleUPtr pulseSimple{
        libPulse.simple_new(
            nullptr,
            appName,
            LibPulse::STREAM_PLAYBACK,
            nullptr,
            "Playback",
            &sampleSpec,
            nullptr,
            nullptr,
            &error),
        PulseSimpleFreer{libPulse}};
    if (!pulseSimple)
        throw Error{str::format(
            "pa_simple_new(): {}", libPulse.strerror(error))};

    activeAudioData = &audioData;
    stopPromise = {};
    playFuture = std::async(
        std::launch::async,
        playTask,
        std::ref(libPulse),
        std::move(pulseSimple),
        std::ref(audioData),
        stopPromise.get_future());
}


void Context::stop()
{
    if (!activeAudioData)
        return;

    stopPromise.set_value();
    assert(playFuture.valid());
    playFuture.get();
    activeAudioData = {};
}


void Context::playTask(
    const LibPulse& libPulse,
    PulseSimpleUPtr pulseSimple,
    const AudioData& audioData,
    std::future<void> stopFuture)
{
    assert(pulseSimple);
    assert(stopFuture.valid());

    const auto writesPerSec = 10;
    std::size_t samplesPerWrite =
        audioData.numChannels * audioData.rate / writesPerSec;
    // Make sure we write full frames.
    if (auto rem = samplesPerWrite % audioData.numChannels)
        samplesPerWrite += audioData.numChannels - rem;

    std::size_t sampleIdx{};
    while (sampleIdx < audioData.samples.size()) {
        if (stopFuture.wait_for(std::chrono::seconds{})
                == std::future_status::ready) {
            libPulse.simple_flush(pulseSimple.get(), nullptr);
            return;
        }

        const auto numSamples = std::min(
            samplesPerWrite, audioData.samples.size() - sampleIdx);

        if (libPulse.simple_write(
                pulseSimple.get(),
                audioData.samples.data() + sampleIdx,
                numSamples * sizeof(*audioData.samples.data()),
                nullptr) < 0)
            return;

        sampleIdx += numSamples;
    }

    libPulse.simple_drain(pulseSimple.get(), nullptr);
}


class PulseAudioPlayer : public Player {
public:
    explicit PulseAudioPlayer(const char* filePath)
        : audioData{sndfile::loadAudioData(filePath)}
    {
    }

    ~PulseAudioPlayer()
    {
        ctx->stop(audioData);
    }

    void play() override
    {
        ctx->play(audioData);
    }
private:
    std::shared_ptr<Context> ctx{Context::get()};
    AudioData audioData;
};


bool checkIsAvailable()
{
    try {
        (void)LibPulse::get();
        (void)LibSndfile::get();
        return true;
    } catch (Error&) {
        return false;
    }
}


}


bool isAvailable()
{
    static const auto result = checkIsAvailable();
    return result;
}


const char* getSystemSoundsDirPath()
{
    // The Sound Theme Specification [1] says that theme directories
    // should be searched under $XDG_DATA_DIRS/sounds, but we should
    // pick a single folder here.
    // [1]: https://www.freedesktop.org/wiki/Specifications/sound-theme-spec
    return "/usr/share/sounds";
}


const std::vector<FormatInfo>& getSupportedFormats()
{
    static const auto result = sndfile::getSupportedFormats();
    return result;
}


std::unique_ptr<Player> Player::create(const char* filePath)
{
    return std::make_unique<PulseAudioPlayer>(filePath);
}


}
