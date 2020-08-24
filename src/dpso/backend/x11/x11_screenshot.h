
#pragma once

#include <memory>

#include <X11/Xlib.h>

#include "backend/screenshot.h"
#include "geometry.h"


namespace dpso {
namespace backend {


class X11Screenshot : public Screenshot {
public:
    static std::unique_ptr<X11Screenshot> take(Display* display, const Rect& rect);

    ~X11Screenshot();

    int getWidth() const override;
    int getHeight() const override;

    void getGrayscaleData(std::uint8_t* buf, int pitch) const override;
private:
    XImage* image;

    explicit X11Screenshot(XImage* image);
};


}
}
