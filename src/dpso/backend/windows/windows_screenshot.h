
#pragma once

#include "backend/backend.h"


namespace dpso {
namespace backend {


class WindowsScreenshot : public Screenshot {
public:
    static WindowsScreenshot* take(const Rect& rect);

    ~WindowsScreenshot();

    int getWidth() const override;
    int getHeight() const override;

    void getGrayscaleData(
        std::uint8_t* buf, int pitch) const override;
private:
    std::uint8_t* buf;
    int w;
    int h;
    int pitch;

    WindowsScreenshot(std::uint8_t* buf, int w, int h, int pitch);
};


}
}
