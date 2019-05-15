
#pragma once

#include <memory>

#include "backend/backend.h"


namespace dpso {
namespace backend {


class WindowsScreenshot : public Screenshot {
public:
    static WindowsScreenshot* take(const Rect& rect);

    int getWidth() const override;
    int getHeight() const override;

    void getGrayscaleData(
        std::uint8_t* buf, int pitch) const override;
private:
    using BufPtr = std::unique_ptr<std::uint8_t[]>;

    BufPtr buf;
    int w;
    int h;
    int pitch;

    WindowsScreenshot(BufPtr data, int w, int h, int pitch);
};


}
}
