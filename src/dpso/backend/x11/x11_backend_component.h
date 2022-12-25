
#pragma once

#include <X11/Xlib.h>


namespace dpso::backend {


class X11BackendComponent {
public:
    virtual ~X11BackendComponent() = default;

    virtual void updateStart() {}
    virtual void handleEvent(const XEvent& event) { (void)event; }
    virtual void updateEnd() {}
};


}
