#pragma once

#include <X11/Xlib.h>


namespace dpso::backend::x11 {


class BackendComponent {
public:
    virtual ~BackendComponent() = default;

    virtual void updateStart() {}
    virtual void handleEvent(const XEvent& event) { (void)event; }
};


}
