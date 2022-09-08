
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "backend/selection.h"
#include "backend/windows/utils/gdi.h"
#include "backend/windows/utils/window.h"


namespace dpso {
namespace backend {


class WindowsSelection : public Selection {
public:
    explicit WindowsSelection(HINSTANCE instance);

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;

    void setBorderWidth(int newBorderWidth) override;

    Rect getGeometry() const override;

    void update();
private:
    bool isEnabled;
    int dpi;
    int baseBorderWidth;
    int borderWidth;
    Point origin;
    Rect geom;

    windows::WindowUPtr window;

    DWORD dashPenPattern[2];
    windows::ObjectUPtr<HPEN> pens[2];

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
}
