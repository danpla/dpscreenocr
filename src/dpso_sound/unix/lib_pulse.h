
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>


namespace dpso::sound {


class LibPulse {
    LibPulse();
public:
    static const LibPulse& get();

    // All names are from libpulse, but without the "pa_" prefix.

    enum stream_direction_t {
        STREAM_PLAYBACK = 1
    };

    enum sample_format_t {
        SAMPLE_S16LE = 3,
        SAMPLE_S16BE,
    };

    static const auto SAMPLE_S16NE =
        #if DPSO_SOUND_IS_BIG_ENDIAN
        SAMPLE_S16BE
        #else
        SAMPLE_S16LE
        #endif
    ;

    struct sample_spec {
        sample_format_t format;
        std::uint32_t rate;
        std::uint8_t channels;
    };

    struct buffer_attr;
    struct channel_map;

    struct simple;

    const char* (*strerror)(int error);
    char* (*get_binary_name)(char* s, std::size_t l);

    simple* (*simple_new)(
        const char* server,
        const char* name,
        stream_direction_t dir,
        const char* dev,
        const char* stream_name,
        const sample_spec* ss,
        const channel_map* map,
        const buffer_attr* attr,
        int* error);
    void (*simple_free)(simple* s);
    int (*simple_write)(
        simple* s,
        const void* data,
        std::size_t bytes,
        int* error);
    int (*simple_drain)(simple* s, int* error);
    int (*simple_flush)(simple* s, int* error);
};


struct PulseSimpleFreer {
    explicit PulseSimpleFreer(const LibPulse& libPulse)
        : libPulse{libPulse}
    {
    }

    void operator()(LibPulse::simple* simple) const
    {
        if (simple)
            libPulse.simple_free(simple);
    }
private:
    const LibPulse& libPulse;
};


using PulseSimpleUPtr =
    std::unique_ptr<LibPulse::simple, PulseSimpleFreer>;


}
