
#include "backend/x11/x11_key_manager.h"

#include <iterator>

#include <X11/keysym.h>


namespace dpso::backend {


// Mod1Mask - Alt
// Mod2Mask - Num Lock
// Mod3Mask - Scroll Lock
// Mod4Mask - Super
// Mod5Mask - ???


static KeyCode keyToKeyCode(Display* display, DpsoKey key);
static DpsoKeyMods x11ModsToDpsoMods(unsigned x11Mods);
static unsigned dpsoModsToX11Mods(DpsoKeyMods dpsoMods);


static void changeBindingGrab(
    Display* display, const X11HotkeyBinding& x11binding, bool grab)
{
    const auto x11Mods = dpsoModsToX11Mods(
        x11binding.binding.hotkey.mods);

    // X11 treats lock keys as modifiers, so we need to register not
    // only the given hotkey, but also its variants with all possible
    // combinations of the lock keys. Since x11ModsToDpsoMods() will
    // skip the lock keys, all the additional helper hotkeys will map
    // to the original one.
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
    , hotkeyAction{dpsoNoHotkeyAction}
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
        hotkeyAction = dpsoNoHotkeyAction;

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
    hotkeyAction = dpsoNoHotkeyAction;
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


const KeySym keyToKeySym[] = {
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

    XK_minus,
    XK_equal,
    XK_BackSpace,

    XK_bracketleft,
    XK_bracketright,
    XK_backslash,

    XK_semicolon,
    XK_apostrophe,
    XK_Return,

    XK_comma,
    XK_period,
    XK_slash,

    XK_Escape,
    XK_grave,
    XK_Tab,

    XK_space,

    XK_Print,
    XK_Pause,
    XK_Insert,
    XK_Delete,
    XK_Home,
    XK_End,
    XK_Prior,
    XK_Next,
    XK_Up,
    XK_Down,
    XK_Left,
    XK_Right,

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
static_assert(std::size(keyToKeySym) == dpsoNumKeys);


static KeyCode keyToKeyCode(Display* display, DpsoKey key)
{
    if (key < 0 || key >= dpsoNumKeys)
        return 0;

    return XKeysymToKeycode(display, keyToKeySym[key]);
}


static DpsoKeyMods x11ModsToDpsoMods(unsigned x11Mods)
{
    DpsoKeyMods dpsoMods = dpsoNoKeyMods;

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


static unsigned dpsoModsToX11Mods(DpsoKeyMods dpsoMods)
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
