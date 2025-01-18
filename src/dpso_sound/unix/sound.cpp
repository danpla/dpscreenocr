
#include "sound.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <functional>
#include <future>
#include <optional>
#include <utility>
#include <vector>

#include "dpso_utils/str.h"

#include "unix/lib_pulse.h"
#include "unix/lib_sndfile.h"


namespace dpso::sound {
namespace {


struct AudioData {
    int numChannels;
    int rate;
    std::vector<short> samples;
};


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
    while (true) {
        if (stopFuture.wait_for(std::chrono::seconds{})
                == std::future_status::ready) {
            libPulse.simple_flush(pulseSimple.get(), nullptr);
            return;
        }

        const auto numSamples = std::min(
            samplesPerWrite, audioData.samples.size() - sampleIdx);
        if (numSamples == 0)
            break;

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


AudioData loadData(const char* filePath)
{
    const auto& libSndfile = LibSndfile::get();

    LibSndfile::INFO info;
    SndfileUPtr sndfile{
        libSndfile.open(filePath, LibSndfile::M_READ, &info),
        SndfileCloser{libSndfile}};
    if (!sndfile)
        throw Error{str::format(
            "sf_open(): {}", libSndfile.strerror(nullptr))};

    libSndfile.command(
        sndfile.get(),
        LibSndfile::C_SET_SCALE_FLOAT_INT_READ,
        nullptr,
        true);
    libSndfile.command(
        sndfile.get(), LibSndfile::C_SET_CLIPPING, nullptr, true);

    AudioData result{info.channels, info.samplerate, {}};

    result.samples.resize(info.frames * info.channels);
    if (libSndfile.readf_short(
                sndfile.get(), result.samples.data(), info.frames)
            != info.frames)
        throw Error{str::format(
            "sf_readf_short(): {}",
            libSndfile.strerror(sndfile.get()))};

    return result;
}


class PulseAudioPlayer : public Player {
public:
    explicit PulseAudioPlayer(const char* filePath)
        : audioData{loadData(filePath)}
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


}


bool isAvailable()
{
    static std::optional<bool> result;
    if (result)
        return *result;

    try {
        (void)LibPulse::get();
        (void)LibSndfile::get();
        result = true;
    } catch (Error&) {
        result = false;
    }

    return *result;
}


std::unique_ptr<Player> Player::create(const char* filePath)
{
    return std::make_unique<PulseAudioPlayer>(filePath);
}



}
