
#include "backend/x11/x11_key_manager.h"

#include <X11/keysym.h>


namespace dpso {
namespace backend {


static KeyCode keyToKeyCode(Display* display, DpsoKey key);
static unsigned x11ModsToDpsoMods(unsigned x11Mods);
static unsigned dpsoModsToX11Mods(unsigned dpsoMods);


static void changeBindingGrab(
    Display* display, const X11HotkeyBinding& x11binding, bool grab)
{
    const auto x11Mods = dpsoModsToX11Mods(
        x11binding.binding.hotkey.mods);

    for (int i = 0; i < 16; ++i) {
        unsigned ignoredX11Mods = 0;
        if (i & 1)
            ignoredX11Mods |= LockMask;
        if (i & 2)
            ignoredX11Mods |= Mod2Mask;
        if (i & 4)
            ignoredX11Mods |= Mod3Mask;
        if (i & 8)
            ignoredX11Mods |= Mod5Mask;

        if (grab)
            XGrabKey(
                display,
                x11binding.keyCode,
                x11Mods | ignoredX11Mods,
                XDefaultRootWindow(display),
                False,
                GrabModeAsync,
                GrabModeAsync);
        else
            XUngrabKey(
                display,
                x11binding.keyCode,
                x11Mods | ignoredX11Mods,
                XDefaultRootWindow(display));
    }
}


X11KeyManager::X11KeyManager(Display* display)
    : display{display}
    , x11bindings{}
    , hotkeysEnabled{}
    , hotkeyAction{-1}
{

}


X11KeyManager::~X11KeyManager()
{
    // Make sure we ungrab everything.
    setHotkeysEnabled(false);
}


bool X11KeyManager::getHotkeysEnabled() const
{
    return hotkeysEnabled;
}


void X11KeyManager::setHotkeysEnabled(bool newHotkeysEnabled)
{
    if (newHotkeysEnabled == hotkeysEnabled)
        return;

    hotkeysEnabled = newHotkeysEnabled;

    if (!hotkeysEnabled)
        hotkeyAction = -1;

    for (const auto& x11binding : x11bindings)
        changeBindingGrab(display, x11binding, hotkeysEnabled);
}


DpsoHotkeyAction X11KeyManager::getLastHotkeyAction() const
{
    return hotkeyAction;
}


bool X11KeyManager::bindHotkey(
    const DpsoHotkey& hotkey, DpsoHotkeyAction action)
{
    for (auto& x11binding : x11bindings)
        if (x11binding.binding.hotkey == hotkey) {
            x11binding.binding.action = action;
            return true;
        }

    const auto keyCode = keyToKeyCode(display, hotkey.key);
    if (keyCode == 0)
        return false;

    x11bindings.push_back({{hotkey, action}, keyCode});

    if (hotkeysEnabled)
        changeBindingGrab(display, x11bindings.back(), true);

    return true;
}


int X11KeyManager::getNumBindings() const
{
    return x11bindings.size();
}


HotkeyBinding X11KeyManager::getBinding(int idx) const
{
    return x11bindings[idx].binding;
}


void X11KeyManager::removeBinding(int idx)
{
    if (hotkeysEnabled)
        changeBindingGrab(display, x11bindings[idx], false);

    if (idx + 1 < static_cast<int>(x11bindings.size()))
        x11bindings[idx] = x11bindings.back();

    x11bindings.pop_back();
}


void X11KeyManager::updateStart()
{
    hotkeyAction = -1;
}


void X11KeyManager::handleEvent(const XEvent& event)
{
    if (!hotkeysEnabled
            || event.type != KeyPress
            || event.xkey.window != XDefaultRootWindow(display))
        return;

    const auto mods = x11ModsToDpsoMods(event.xkey.state);

    for (const auto& x11binding : x11bindings)
        if (x11binding.keyCode == event.xkey.keycode
                && x11binding.binding.hotkey.mods == mods) {
            hotkeyAction = x11binding.binding.action;
            break;
        }
}


const KeySym keyToKeySym[dpsoNumKeys] = {
    XK_a,
    XK_b,
    XK_c,
    XK_d,
    XK_e,
    XK_f,
    XK_g,
    XK_h,
    XK_i,
    XK_j,
    XK_k,
    XK_l,
    XK_m,
    XK_n,
    XK_o,
    XK_p,
    XK_q,
    XK_r,
    XK_s,
    XK_t,
    XK_u,
    XK_v,
    XK_w,
    XK_x,
    XK_y,
    XK_z,

    XK_1,
    XK_2,
    XK_3,
    XK_4,
    XK_5,
    XK_6,
    XK_7,
    XK_8,
    XK_9,
    XK_0,

    XK_Return,
    XK_Escape,
    XK_BackSpace,
    XK_Tab,
    XK_space,

    XK_minus,
    XK_equal,
    XK_bracketleft,
    XK_bracketright,

    XK_semicolon,
    XK_apostrophe,
    XK_grave,
    XK_comma,
    XK_period,
    XK_slash,

    XK_F1,
    XK_F2,
    XK_F3,
    XK_F4,
    XK_F5,
    XK_F6,
    XK_F7,
    XK_F8,
    XK_F9,
    XK_F10,
    XK_F11,
    XK_F12,

    XK_Print,
    XK_Pause,
    XK_Insert,
    XK_Home,
    XK_Prior,
    XK_Delete,
    XK_End,
    XK_Next,
    XK_Right,
    XK_Left,
    XK_Down,
    XK_Up,

    XK_KP_Divide,
    XK_KP_Multiply,
    XK_KP_Subtract,
    XK_KP_Add,
    XK_KP_Enter,
    XK_KP_End,
    XK_KP_Down,
    XK_KP_Next,
    XK_KP_Left,
    XK_KP_Begin,
    XK_KP_Right,
    XK_KP_Home,
    XK_KP_Up,
    XK_KP_Prior,
    XK_KP_Insert,
    XK_KP_Delete,
};


static KeyCode keyToKeyCode(Display* display, DpsoKey key)
{
    if (key < 0 || key >= dpsoNumKeys)
        return 0;

    const auto keySym = keyToKeySym[key];
    return XKeysymToKeycode(display, keySym);
}


// Mod1Mask - Alt
// Mod2Mask - Num Lock
// Mod3Mask - Scroll Lock
// Mod4Mask - Super
// Mod5Mask - ???


static unsigned x11ModsToDpsoMods(unsigned x11Mods)
{
    unsigned dpsoMods = dpsoKeyModNone;

    if (x11Mods & ShiftMask)
        dpsoMods |= dpsoKeyModShift;
    if (x11Mods & ControlMask)
        dpsoMods |= dpsoKeyModCtrl;
    if (x11Mods & Mod1Mask)
        dpsoMods |= dpsoKeyModAlt;
    if (x11Mods & Mod4Mask)
        dpsoMods |= dpsoKeyModWin;

    return dpsoMods;
}


static unsigned dpsoModsToX11Mods(unsigned dpsoMods)
{
    unsigned x11Mods = 0;

    if (dpsoMods & dpsoKeyModShift)
        x11Mods |= ShiftMask;
    if (dpsoMods & dpsoKeyModCtrl)
        x11Mods |= ControlMask;
    if (dpsoMods & dpsoKeyModAlt)
        x11Mods |= Mod1Mask;
    if (dpsoMods & dpsoKeyModWin)
        x11Mods |= Mod4Mask;

    return x11Mods;
}


}
}
