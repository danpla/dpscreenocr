#pragma once


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    DpsoPxFormatGrayscale,
    DpsoPxFormatRgb,
    DpsoPxFormatBgr,
    DpsoPxFormatRgba,
    DpsoPxFormatBgra,
    DpsoPxFormatArgb,
    DpsoPxFormatAbgr,
} DpsoPxFormat;


const char* dpsoPxFormatToStr(DpsoPxFormat pxFormat);


int dpsoPxFormatGetBytesPerPx(DpsoPxFormat pxFormat);


#ifdef __cplusplus
}
#endif
