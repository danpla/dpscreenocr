#pragma once

#include <X11/Xlib.h>


namespace dpso::backend::x11 {


class BackendComponent {
public:
    virtual ~BackendComponent() = default;

    virtual void updateStart() {}

    // Return true if the event was consumed.
    virtual bool handleEvent(const XEvent& event) = 0;
};


}
