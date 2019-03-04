
#pragma once

#include <X11/Xlib.h>

#include "backend/backend.h"


namespace dpso {
namespace backend {


class X11Screenshot : public Screenshot {
public:
    static X11Screenshot* take(Display* display, const Rect& rect);

    ~X11Screenshot();

    int getWidth() const override;
    int getHeight() const override;

    void getGrayscaleData(std::uint8_t* buf, int pitch) const override;
private:
    XImage* image;

    X11Screenshot(XImage* image);
};


}
}
