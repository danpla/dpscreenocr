
#pragma once

#include <cstdint>


namespace dpso::backend {


class Screenshot {
public:
    virtual ~Screenshot() = default;

    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;

    virtual void getGrayscaleData(
        std::uint8_t* buf, int pitch) const = 0;
};


}
