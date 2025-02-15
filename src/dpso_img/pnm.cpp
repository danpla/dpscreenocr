#include "pnm.h"

#include <array>
#include <vector>

#include "dpso_utils/str.h"
#include "dpso_utils/stream/file_stream.h"
#include "dpso_utils/stream/utils.h"


namespace dpso::img {


const char* getPnmExt(DpsoPxFormat pxFormat)
{
    return pxFormat == DpsoPxFormatGrayscale ? ".pgm" : ".ppm";
}


static std::array<int, 3> getRgbIndices(DpsoPxFormat pxFormat)
{
    switch (pxFormat) {
    case DpsoPxFormatGrayscale:
        break;
    case DpsoPxFormatRgb:
        return {0, 1, 2};
    case DpsoPxFormatBgr:
        return {2, 1, 0};
    case DpsoPxFormatRgba:
        return {0, 1, 2};
    case DpsoPxFormatBgra:
        return {2, 1, 0};
    case DpsoPxFormatArgb:
        return {1, 2, 3};
    case DpsoPxFormatAbgr:
        return {3, 2, 1};
    }

    return {};
}


static void writePnm(
    Stream& stream,
    DpsoPxFormat pxFormat,
    const std::uint8_t* data,
    int w,
    int h,
    int pitch)
{
    const auto bpp = dpsoPxFormatGetBytesPerPx(pxFormat);

    if (w < 1 || h < 1 || pitch < w * bpp)
        return;

    write(stream, str::format(
        "P{}\n{} {}\n255\n",
        pxFormat == DpsoPxFormatGrayscale ? '5' : '6', w, h));

    if (pxFormat == DpsoPxFormatGrayscale
            || pxFormat == DpsoPxFormatRgb) {
        for (int y = 0; y < h; ++y)
            stream.write(data + y * pitch, w * bpp);
        return;
    }

    const auto rgbIndices = getRgbIndices(pxFormat);
    std::vector<std::uint8_t> rgbRow(w * 3);

    for (int y = 0; y < h; ++y) {
        const auto* src = data + y * pitch;
        auto* dst = rgbRow.data();
        for (int x = 0; x < w; ++x) {
            for (int i = 0; i < 3; ++i)
                dst[i] = src[rgbIndices[i]];

            src += bpp;
            dst += 3;
        }

        stream.write(rgbRow.data(), rgbRow.size());
    }
}


void savePnm(
    const char* filePath,
    DpsoPxFormat pxFormat,
    const std::uint8_t* data,
    int w,
    int h,
    int pitch)
{
    try {
        FileStream file{filePath, FileStream::Mode::write};
        writePnm(file, pxFormat, data, w, h, pitch);
    } catch (std::runtime_error&) {  // os::Error, StreamError
    }
}


}
