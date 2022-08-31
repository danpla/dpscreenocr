
#pragma once

#include <X11/Xlib.h>

#include "backend/selection.h"
#include "backend/x11/x11_backend_component.h"


namespace dpso {
namespace backend {


class X11Selection : public Selection, public X11BackendComponent {
public:
    explicit X11Selection(Display* display);
    ~X11Selection();

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;

    void setBorderWidth(int newBorderWidth) override;

    Rect getGeometry() const override;

    void updateStart() override;
    void handleEvent(const XEvent& event) override;
private:
    Display* display;
    Window window;
    GC gc;

    bool isEnabled;
    int baseBorderWidth;
    int borderWidth;
    Point origin;
    Rect geom;

    void calcBorderWidth();
    void setGeometry(const Rect& newGeom);
    void reshapeWindow();
    void drawSelection();
};


}
}
