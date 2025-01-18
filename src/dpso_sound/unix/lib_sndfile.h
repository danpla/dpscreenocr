
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
        C_SET_SCALE_FLOAT_INT_READ = 0x1014,
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
