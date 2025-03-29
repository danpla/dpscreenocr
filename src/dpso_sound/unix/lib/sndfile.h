#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>


namespace dpso::sound {


class LibSndfile {
    LibSndfile();
public:
    static const LibSndfile& get();

    // All names are from libsndfile, but without the "sf_" prefix.

    enum {
        FORMAT_WAV = 0x010000,
        FORMAT_AIFF = 0x020000,
        FORMAT_FLAC = 0x170000,
        FORMAT_CAF = 0x180000,
        FORMAT_OGG = 0x200000,

        FORMAT_FLOAT = 0x0006,
        FORMAT_DOUBLE = 0x0007,
        FORMAT_VORBIS = 0x0060,
        FORMAT_OPUS = 0x0064,
        FORMAT_MPEG_LAYER_III = 0x0082,

        FORMAT_SUBMASK = 0x0000FFFF
    };

    enum {
        C_SET_SCALE_FLOAT_INT_READ = 0x1014,
        C_GET_SIMPLE_FORMAT_COUNT = 0x1020,
        C_GET_SIMPLE_FORMAT = 0x1021,
        C_SET_CLIPPING = 0x10C0
    };

    enum {
        M_READ = 0x10,
    };

    struct SNDFILE;

    using count_t = std::int64_t;

    struct INFO {
        count_t frames;
        int samplerate;
        int channels;
        int format;
        int sections;
        int seekable;
    };

    struct FORMAT_INFO {
        int format;
        const char *name;
        const char *extension;
    };

    SNDFILE* (*open)(const char* path, int mode, INFO* info);
    int (*command)(
        SNDFILE* sndfile, int cmd, void* data, int datasize);
    count_t (*readf_short)(
        SNDFILE* sndfile, short* ptr, count_t frames);
    int (*close)(SNDFILE* sndfile);

    const char* (*strerror)(SNDFILE *sndfile);
};


struct SndfileCloser {
    explicit SndfileCloser(const LibSndfile& libSndfile)
        : libSndfile{libSndfile}
    {
    }

    void operator()(LibSndfile::SNDFILE* sndfile) const
    {
        if (sndfile)
            libSndfile.close(sndfile);
    }
private:
    const LibSndfile& libSndfile;
};


using SndfileUPtr =
    std::unique_ptr<LibSndfile::SNDFILE, SndfileCloser>;


}
