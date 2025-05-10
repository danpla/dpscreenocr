#pragma once

#include <memory>
#include <utility>

#include <X11/Xlib.h>
#include <X11/Xutil.h>


namespace dpso::backend {


struct DisplayCloser {
    void operator()(Display* display) const
    {
        if (display)
            XCloseDisplay(display);
    }
};


using DisplayUPtr = std::unique_ptr<Display, DisplayCloser>;


struct ImageDestroyer {
    void operator()(XImage* image) const
    {
        if (image)
            XDestroyImage(image);
    }
};


using ImageUPtr = std::unique_ptr<XImage, ImageDestroyer>;


template<typename Resource, auto deleter>
class ResourceHandle {
public:
    ResourceHandle() = default;

    ResourceHandle(Display* display, Resource resource)
        : display{display}
        , resource{resource}
    {
    }

    ~ResourceHandle()
    {
        if (display && resource)
            deleter(display, resource);
    }

    ResourceHandle(const ResourceHandle&) = delete;
    ResourceHandle& operator=(const ResourceHandle&) = delete;

    ResourceHandle(ResourceHandle&& other) noexcept
        : display{std::exchange(other.display, {})}
        , resource{std::exchange(other.resource, {})}
    {
    }

    ResourceHandle& operator=(ResourceHandle&& other) noexcept
    {
        if (this != &other) {
            if (display && resource)
                deleter(display, resource);

            display = std::exchange(other.display, {});
            resource = std::exchange(other.resource, {});
        }

        return *this;
    }

    operator Resource() const
    {
        return resource;
    }
private:
    Display* display{};
    Resource resource{};
};


using WindowHandle = ResourceHandle<Window, XDestroyWindow>;
using GcHandle = ResourceHandle<GC, XFreeGC>;


}
