#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "backend/selection.h"
#include "dpso_utils/windows/gdi.h"
#include "dpso_utils/windows/window.h"


namespace dpso::backend::windows {


class Selection : public backend::Selection {
public:
    explicit Selection(HINSTANCE instance);
    ~Selection();

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;

    void setBorderWidth(int newBorderWidth) override;

    Rect getGeometry() const override;

    void update();
private:
    bool isEnabled{};
    int dpi{};
    int baseBorderWidth{defaultBorderWidth};
    int borderWidth{baseBorderWidth};
    Point origin;
    Rect geom;

    dpso::windows::WindowUPtr window;

    dpso::windows::ObjectUPtr<HPEN> pens[2];

    static LRESULT CALLBACK wndProc(
        HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void updateBorderWidth();
    void updatePens();
    void updateWindowGeometry();
    void updateWindowRegion();
    void setGeometry(const Rect& newGeom);
    void draw(HDC dc);
};


}
