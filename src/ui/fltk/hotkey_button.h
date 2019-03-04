
#pragma once

#include <FL/Fl.H>
#include <FL/Fl_Button.H>


class HotkeyButton : public Fl_Button {
public:
    HotkeyButton(
        int x, int y, int w, int h,
        int action);

    int handle(int event) override;
private:
    int action;
};
