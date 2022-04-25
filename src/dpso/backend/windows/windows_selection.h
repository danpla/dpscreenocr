
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "backend/selection.h"
#include "backend/windows/utils.h"


namespace dpso {
namespace backend {


class WindowsSelection : public Selection {
public:
    explicit WindowsSelection(HINSTANCE instance);

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;

    Rect getGeometry() const override;

    void update();
private:
    int borderWidth;
    bool isEnabled;
    Point origin;
    Rect geom;

    windows::WindowPtr window;

    DWORD dashPenPattern[2];
    windows::ObjectPtr<HPEN> pens[2];

    static LRESULT CALLBACK wndProc(
        HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void createPens();
    void setGeometry(const Rect& newGeom);
    void draw(HDC dc);
    void updateWindowRegion();
};


}
}
