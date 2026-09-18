#pragma once

#include <X11/Xlib.h>

#include "dpso_utils/geometry.h"

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
    bool handleEvent(const XEvent& event) override;
    void updateEnd() override;
private:
    Display* display;
    Window rootWindow;
    int screenNum;

    WindowHandle window;

    bool isEnabled{};
    Point origin;
    Rect geom;

    int baseBorderWidth{defaultBorderWidth};
    int borderWidth{baseBorderWidth};
    int dashLen{borderWidth * squaresPerDash};

    GcHandle tileGc;
    PixmapHandle xTile;
    PixmapHandle yTile;

    static const int numBorders{4};
    XRectangle borderRects[numBorders];
    GcHandle borderGcs[numBorders];

    bool needRedraw{};

    void updateBorderProperties();
    void updateWindow();
    void draw();
};


}
