
#pragma once

#include <cstdint>


namespace dpso {
namespace backend {


class Screenshot {
public:
    Screenshot() = default;
    virtual ~Screenshot() = default;

    Screenshot(const Screenshot& other) = delete;
    Screenshot& operator=(const Screenshot& other) = delete;

    Screenshot(Screenshot&& other) = delete;
    Screenshot& operator=(Screenshot&& other) = delete;

    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;

    virtual void getGrayscaleData(std::uint8_t* buf, int pitch) const = 0;
};


}
}
