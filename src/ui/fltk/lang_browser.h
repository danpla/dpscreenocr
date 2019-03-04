
#pragma once

#include <FL/Fl.H>
#include <FL/Fl_Check_Browser.H>


class LangBrowser : public Fl_Check_Browser {
public:
    LangBrowser(int x, int y, int w, int h, const char* l);
};
