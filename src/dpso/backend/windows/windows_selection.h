
#pragma once

#include <windows.h>

#include "backend/backend.h"
#include "backend/windows/utils.h"


namespace dpso {
namespace backend {


class WindowsSelection : public Selection {
public:
    WindowsSelection();

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;

    Rect getGeometry() const override;

    void update();
private:
    int borderWidth;
    bool isEnabled;
    Point origin;
    Rect geom;

    WindowPtr window;

    DWORD dashPenPattern[2];
    ObjectPtr<HPEN> pens[numPens];

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
