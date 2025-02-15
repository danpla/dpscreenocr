#include "px_format.h"


const char* dpsoPxFormatToStr(DpsoPxFormat pxFormat)
{
    switch (pxFormat) {
    case DpsoPxFormatGrayscale:
        return "Grayscale";
    case DpsoPxFormatRgb:
        return "RGB";
    case DpsoPxFormatBgr:
        return "BGR";
    case DpsoPxFormatRgba:
        return "RGBA";
    case DpsoPxFormatBgra:
        return "BGRA";
    case DpsoPxFormatArgb:
        return "ARGB";
    case DpsoPxFormatAbgr:
        return "ABGR";
    }

    return "";
}


int dpsoPxFormatGetBytesPerPx(DpsoPxFormat pxFormat)
{
    switch (pxFormat) {
    case DpsoPxFormatGrayscale:
        return 1;
    case DpsoPxFormatRgb:
    case DpsoPxFormatBgr:
        return 3;
    case DpsoPxFormatRgba:
    case DpsoPxFormatBgra:
    case DpsoPxFormatArgb:
    case DpsoPxFormatAbgr:
        return 4;
    }

    return {};
}
