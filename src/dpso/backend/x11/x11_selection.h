
#pragma once

#include <X11/Xlib.h>

#include "backend/backend.h"
#include "backend/x11/x11_backend_component.h"


namespace dpso {
namespace backend {


class X11Selection : public Selection, public X11BackendComponent {
public:
    X11Selection(Display* display);
    ~X11Selection();

    bool getIsEnabled() const override;
    void setIsEnabled(bool newEnabled) override;

    Rect getGeometry() const override;

    void updateStart() override;
    void handleEvent(const XEvent& event) override;
private:
    Display* display;
    Window window;
    GC gc;

    bool isEnabled;
    Point origin;
    Rect geom;

    void setGeometry(const Rect& newGeom, bool force);
    void reshapeWindow();
    void drawSelection();
};


}
}
