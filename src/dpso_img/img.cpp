#include "img.h"

#include <cstddef>
#include <cstdint>
#include <memory>

#include "dpso_utils/error_set.h"


using namespace dpso;


struct DpsoImg {
    DpsoPxFormat pxFormat;
    int w;
    int h;
    int pitch;
    std::unique_ptr<std::uint8_t[]> data;
};


DpsoImg* dpsoImgCreate(
    DpsoPxFormat pxFormat, int w, int h, int pitch)
{
    if (w < 1) {
        setError("w < 1");
        return {};
    }

    if (h < 1) {
        setError("h < 1");
        return {};
    }

    const auto minPitch = w * dpsoPxFormatGetBytesPerPx(pxFormat);

    if (pitch == 0)
        pitch = minPitch;
    else if (pitch < minPitch) {
        setError(
            "pitch < {} (w * {})",
            minPitch, dpsoPxFormatGetBytesPerPx(pxFormat));
        return {};
    }

    return new DpsoImg{
        pxFormat,
        w,
        h,
        pitch,
        // We don't use std::make_unique so that the array is not
        // value-initialized.
        std::unique_ptr<std::uint8_t[]>{
            new std::uint8_t[static_cast<std::size_t>(pitch) * h]}};
}


void dpsoImgDelete(DpsoImg* img)
{
    delete img;
}


DpsoPxFormat dpsoImgGetPxFormat(const DpsoImg* img)
{
    return img ? img->pxFormat : DpsoPxFormat{};
}


int dpsoImgGetWidth(const DpsoImg* img)
{
    return img ? img->w : 0;
}


int dpsoImgGetHeight(const DpsoImg* img)
{
    return img ? img->h : 0;
}


int dpsoImgGetPitch(const DpsoImg* img)
{
    return img ? img->pitch : 0;
}


const uint8_t* dpsoImgGetConstData(const DpsoImg* img)
{
    return img ? img->data.get() : nullptr;
}


uint8_t* dpsoImgGetData(DpsoImg* img)
{
    return img ? img->data.get() : nullptr;
}
