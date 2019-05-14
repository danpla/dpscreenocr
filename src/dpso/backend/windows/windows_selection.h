
#pragma once

#include <windows.h>

#include "backend/backend.h"


namespace dpso {
namespace backend {


class WindowsSelection : public Selection {
public:
    WindowsSelection();
    ~WindowsSelection();

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;

    Rect getGeometry() const override;

    void update();
private:
    int borderWidth;
    bool isEnabled;
    Point origin;
    Rect geom;

    HWND window;

    static const int numPens = 2;
    HPEN pens[numPens];

    static LRESULT CALLBACK wndProc(
        HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void throwLastError(const char* description);

    bool createPens();
    void setGeometry(const Rect& newGeom);
    void draw(HDC dc);
    void updateWindowRegion();
};


}
}
