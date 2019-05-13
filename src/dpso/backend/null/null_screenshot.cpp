
#include "backend/null/null_screenshot.h"

#include <cstdio>


namespace dpso {
namespace backend {


#define MSG(...) std::printf("NullScreenshot: " __VA_ARGS__)


NullScreenshot::NullScreenshot(const Rect& rect)
    : w {rect.w}
    , h {rect.h}
{
    MSG("Screenshot %ix%i\n", w, h);
}


int NullScreenshot::getWidth() const
{
    return w;
}


int NullScreenshot::getHeight() const
{
    return h;
}


void NullScreenshot::getGrayscaleData(
    std::uint8_t* buf, int pitch) const
{
    (void)buf;
    MSG("Get grayscale data; pitch %i\n", pitch);
}


}
}
