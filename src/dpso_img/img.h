#pragma once

#include <stdint.h>

#include "px_format.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct DpsoImg DpsoImg;


/**
 * Create a new image.
 *
 * The pitch is the byte size of the image row, which should be at
 * least w * dpsoPxFormatGetBytesPerPx(pxFormat). You can use 0 to
 * automatically calculate and use the minimal pitch.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * null.
 */
DpsoImg* dpsoImgCreate(
    DpsoPxFormat pxFormat, int w, int h, int pitch);


void dpsoImgDelete(DpsoImg* img);


DpsoPxFormat dpsoImgGetPxFormat(const DpsoImg* img);
int dpsoImgGetWidth(const DpsoImg* img);
int dpsoImgGetHeight(const DpsoImg* img);
int dpsoImgGetPitch(const DpsoImg* img);

const uint8_t* dpsoImgGetConstData(const DpsoImg* img);
uint8_t* dpsoImgGetData(DpsoImg* img);


#ifdef __cplusplus
}


#include <memory>


namespace dpso::img {


struct ImgDeleter {
    void operator()(DpsoImg* img) const
    {
        dpsoImgDelete(img);
    }
};


using ImgUPtr = std::unique_ptr<DpsoImg, ImgDeleter>;


}


#endif
