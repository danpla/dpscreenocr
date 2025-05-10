#pragma once

#include <X11/Xlib.h>

#include "backend/selection.h"
#include "backend/unix/x11/backend_component.h"
#include "backend/unix/x11/utils.h"


namespace dpso::backend::x11 {


class Selection : public backend::Selection, public BackendComponent {
public:
    explicit Selection(Display* display);

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;

    void setBorderWidth(int newBorderWidth) override;

    Rect getGeometry() const override;

    void updateStart() override;
    void handleEvent(const XEvent& event) override;
private:
    Display* display;
    WindowHandle window{};
    GcHandle gc{};

    bool isEnabled{};
    int baseBorderWidth{defaultBorderWidth};
    int borderWidth{baseBorderWidth};
    Point origin;
    Rect geom;

    void updateBorderWidth();
    void updateWindowGeometry();
    void updateWindowShape();
    void setGeometry(const Rect& newGeom);
    void draw();
};


}
