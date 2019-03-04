
#pragma once

#include <FL/Fl.H>
#include <FL/Fl_Choice.H>


class LanguageChoice : public Fl_Choice {
public:
    LanguageChoice(int x, int y, int w, int h, const char* l);
};
