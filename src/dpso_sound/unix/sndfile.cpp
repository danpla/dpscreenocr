#include "sndfile.h"

#include "dpso_utils/str.h"

#include "error.h"
#include "unix/lib/sndfile.h"


namespace dpso::sound::sndfile {


// The format info in sndfile is a complete mess. The "major" format
// (SF_FORMAT_TYPEMASK) and the "subtype" (SF_FORMAT_TYPEMASK) can be:
//   * Container + coding format, e.g. Vorbis is
//     SF_FORMAT_OGG | SF_FORMAT_VORBIS
//   * Coding + sample format, e.g. FLAC is
//     SF_FORMAT_FLAC | SF_FORMAT_PCM_16
//   * Just coding format, split between the two fields, e.g. MP3 is
//     SF_FORMAT_MPEG | SF_FORMAT_MPEG_LAYER_III
//
// Since we only need to collect the file extensions, we only work
// with the format field of SF_FORMAT_INFO.
std::vector<FormatInfo> getSupportedFormats(
    const LibSndfile& libSndfile)
{
    struct Format {
        int sfFormat;
        FormatInfo info;
    };

    // We don't want to return all the formats supported by sndfile,
    // because they include the ones that are not widely used
    // nowadays, such as AU and VOX.
    //
    // Note that the ".oga" extension can legitimately be used not
    // only for Vorbis, but also for FLAC and Opus data in an OGG
    // container. In practice, however, it's only important to include
    // ".oga" as an alternative for Vorbis, since the Sound Theme
    // Specification [1] requires this extension instead of ".ogg".
    //
    // 1: https://www.freedesktop.org/wiki/Specifications/sound-theme-spec/
    const std::vector<Format> expectedFormats{
        {LibSndfile::FORMAT_AIFF,
            {"AIFF", {".aiff", ".aif", ".aifc"}}},
        {LibSndfile::FORMAT_CAF, {"CAF", {".caf"}}},
        {LibSndfile::FORMAT_FLAC, {"FLAC", {".flac"}}},
        {LibSndfile::FORMAT_MPEG_LAYER_III, {"MP3", {".mp3"}}},
        {LibSndfile::FORMAT_OPUS, {"Opus", {".opus"}}},
        {LibSndfile::FORMAT_VORBIS, {"Vorbis", {".ogg", ".oga"}}},
        {LibSndfile::FORMAT_WAV, {"WAV", {".wav"}}},
    };

    std::vector<FormatInfo> result;

    int formatCount;
    libSndfile.command(
        nullptr,
        LibSndfile::C_GET_SIMPLE_FORMAT_COUNT,
        &formatCount,
        sizeof(int));

    // We iterate through expectedFormats first because sndfile
    // formats contain duplicates, e.g. multiple WAV for each internal
    // format it supports.
    for (const auto& ef : expectedFormats)
        for (int i = 0; i < formatCount; i++) {
            LibSndfile::FORMAT_INFO formatInfo;
            formatInfo.format = i;

            libSndfile.command(
                nullptr,
                LibSndfile::C_GET_SIMPLE_FORMAT,
                &formatInfo,
                sizeof(formatInfo));

            if (formatInfo.format & ef.sfFormat) {
                result.push_back(ef.info);
                break;
            }
        }

    return result;
}


std::vector<FormatInfo> getSupportedFormats()
{
    try {
        return getSupportedFormats(LibSndfile::get());
    } catch (Error&) {
        return {};
    }
}


AudioData loadAudioData(const char* filePath)
{
    const auto& libSndfile = LibSndfile::get();

    LibSndfile::INFO info;
    SndfileUPtr sndfile{
        libSndfile.open(filePath, LibSndfile::M_READ, &info),
        SndfileCloser{libSndfile}};
    if (!sndfile)
        throw Error{str::format(
            "sf_open(): {}", libSndfile.strerror(nullptr))};

    // The soundfile documentation recommends enabling
    // SFC_SET_SCALE_FLOAT_INT_READ to properly read shorts from files
    // containing floating-point samples in the [-1.0, 1.0] range, but
    // it also affects Vorbis and Opus, making the audio too loud, so
    // we only enable it explicitly for SF_FORMAT_FLOAT/DOUBLE.
    if (const auto format = info.format & LibSndfile::FORMAT_SUBMASK;
            format == LibSndfile::FORMAT_FLOAT
            || format == LibSndfile::FORMAT_DOUBLE)
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


}
