
#include "hotkey_button.h"

#include "dpso.h"


static const int keyToFlKey[dpsoNumKeys] = {
    'a',
    'b',
    'c',
    'd',
    'e',
    'f',
    'g',
    'h',
    'i',
    'j',
    'k',
    'l',
    'm',
    'n',
    'o',
    'p',
    'q',
    'r',
    's',
    't',
    'u',
    'v',
    'w',
    'x',
    'y',
    'z',

    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',

    FL_Enter,
    FL_Escape,
    FL_BackSpace,
    FL_Tab,
    ' ',

    '-',
    '=',
    '[',
    ']',

    ';',
    '\'',
    '`',
    ',',
    '.',
    '/',

    FL_F + 1,
    FL_F + 2,
    FL_F + 3,
    FL_F + 4,
    FL_F + 5,
    FL_F + 6,
    FL_F + 7,
    FL_F + 8,
    FL_F + 9,
    FL_F + 10,
    FL_F + 11,
    FL_F + 12,

    FL_Print,
    FL_Pause,
    FL_Insert,
    FL_Home,
    FL_Page_Up,
    FL_Delete,
    FL_End,
    FL_Page_Down,
    FL_Right,
    FL_Left,
    FL_Down,
    FL_Up,

    FL_KP + '/',
    FL_KP + '*',
    FL_KP + '-',
    FL_KP + '+',
    FL_KP_Enter,
    FL_KP + '1',
    FL_KP + '2',
    FL_KP + '3',
    FL_KP + '4',
    FL_KP + '5',
    FL_KP + '6',
    FL_KP + '7',
    FL_KP + '8',
    FL_KP + '9',
    FL_KP + '0',
    FL_KP + '.',
};


static DpsoKey flKeyToKey(int flKey)
{
    for (int i = 0; i < dpsoNumKeys; ++i)
        if (keyToFlKey[i] == flKey)
            return static_cast<DpsoKey>(i);

    return dpsoUnknownKey;
}


static int flStateToMods(int flState)
{
    int mods = dpsoKeyModNone;

    if (flState & FL_SHIFT)
        mods |= dpsoKeyModShift;
    if (flState & FL_CTRL)
        mods |= dpsoKeyModCtrl;
    if (flState & FL_ALT)
        mods |= dpsoKeyModAlt;
    if (flState & FL_META)
        mods |= dpsoKeyModWin;

    return mods;
}


HotkeyButton::HotkeyButton(
        int x, int y, int w, int h,
        int action)
    : Fl_Button(x, y, w, h)
    , action {action}
{
    when(0);
    type(FL_TOGGLE_BUTTON);

    DpsoKey key;
    int mods;
    if (!dpsoFindActionHotkey(action, &key, &mods))
        return;

    copy_label(dpsoHotkeyToString(key, mods));
}


int HotkeyButton::handle(int event)
{
    if (event == FL_KEYDOWN) {
        if (!value())
            return 0;

        if (Fl::event_key() == FL_Escape) {
            value(0);
            set_changed();
            redraw();
            return 1;
        }

        const auto key = flKeyToKey(Fl::event_key());
        if (key == dpsoUnknownKey)
            return 1;

        const auto mods = flStateToMods(Fl::event_state());

        dpsoBindHotkey(key, mods, action);
        copy_label(dpsoHotkeyToString(key, mods));

        value(0);
        set_changed();
        redraw();

        return 1;
    } else if (event == FL_FOCUS) {
        redraw();
        return 1;
    } else if (event == FL_UNFOCUS) {
        value(0);
        redraw();
        return 1;
    } else {
        const auto r = Fl_Button::handle(event);

        if (event == FL_RELEASE
                && value()
                && Fl::focus() != this) {
            take_focus();
        }

        return r;
    }
}
