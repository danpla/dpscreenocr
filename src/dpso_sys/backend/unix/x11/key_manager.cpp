#include "backend/unix/x11/key_manager.h"

#include <iterator>

#include <X11/keysym.h>


namespace dpso::backend::x11 {


// Mod1Mask - Alt
// Mod2Mask - Num Lock
// Mod3Mask - Scroll Lock
// Mod4Mask - Super
// Mod5Mask - ???


static KeyCode keyToKeyCode(Display* display, DpsoKey key);
static DpsoKeyMods toDpsoMods(unsigned x11Mods);
static unsigned toX11Mods(DpsoKeyMods dpsoMods);


KeyManager::KeyManager(Display* display)
    : display{display}
{
}


KeyManager::~KeyManager()
{
    // Make sure we ungrab everything.
    setIsEnabled(false);
}


bool KeyManager::getIsEnabled() const
{
    return isEnabled;
}


void KeyManager::setIsEnabled(bool newIsEnabled)
{
    if (newIsEnabled == isEnabled)
        return;

    isEnabled = newIsEnabled;

    if (!isEnabled)
        hotkeyAction = dpsoNoHotkeyAction;

    for (const auto& x11binding : x11bindings)
        changeGrab(display, x11binding, isEnabled);
}


DpsoHotkeyAction KeyManager::getLastHotkeyAction() const
{
    return hotkeyAction;
}


void KeyManager::bindHotkey(
    const DpsoHotkey& hotkey, DpsoHotkeyAction action)
{
    for (auto& x11binding : x11bindings)
        if (x11binding.binding.hotkey == hotkey) {
            x11binding.binding.action = action;
            return;
        }

    const auto keyCode = keyToKeyCode(display, hotkey.key);
    if (keyCode == 0)
        return;

    x11bindings.push_back({{hotkey, action}, keyCode});

    if (isEnabled)
        changeGrab(display, x11bindings.back(), true);
}


int KeyManager::getNumBindings() const
{
    return x11bindings.size();
}


HotkeyBinding KeyManager::getBinding(int idx) const
{
    return x11bindings[idx].binding;
}


void KeyManager::removeBinding(int idx)
{
    if (isEnabled)
        changeGrab(display, x11bindings[idx], false);

    if (idx + 1 < static_cast<int>(x11bindings.size()))
        x11bindings[idx] = x11bindings.back();

    x11bindings.pop_back();
}


void KeyManager::updateStart()
{
    hotkeyAction = dpsoNoHotkeyAction;
}


void KeyManager::handleEvent(const XEvent& event)
{
    if (!isEnabled
            || event.type != KeyPress
            || event.xkey.window != XDefaultRootWindow(display))
        return;

    const auto mods = toDpsoMods(event.xkey.state);

    for (const auto& x11binding : x11bindings)
        if (x11binding.keyCode == event.xkey.keycode
                && x11binding.binding.hotkey.mods == mods) {
            hotkeyAction = x11binding.binding.action;
            break;
        }
}


void KeyManager::changeGrab(
    Display* display, const X11HotkeyBinding& x11binding, bool grab)
{
    const auto x11Mods = toX11Mods(
        x11binding.binding.hotkey.mods);

    // X11 treats lock keys as modifiers, so we need to register not
    // only the given hotkey, but also its variants with all possible
    // combinations of the lock keys. Since toDpsoMods() will
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


const KeySym keyToKeySym[]{
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


static const struct {
    unsigned x11Mod;
    DpsoKeyMod dpsoMod;
} modMap[]{
    {ShiftMask, dpsoKeyModShift},
    {ControlMask, dpsoKeyModCtrl},
    {Mod1Mask, dpsoKeyModAlt},
    {Mod4Mask, dpsoKeyModWin},
};
static_assert(std::size(modMap) == dpsoNumKeyMods);


static DpsoKeyMods toDpsoMods(unsigned x11Mods)
{
    DpsoKeyMods dpsoMods = dpsoNoKeyMods;

    for (const auto& modPair : modMap)
        if (x11Mods & modPair.x11Mod)
            dpsoMods |= modPair.dpsoMod;

    return dpsoMods;
}


static unsigned toX11Mods(DpsoKeyMods dpsoMods)
{
    unsigned x11Mods = 0;

    for (const auto& modPair : modMap)
        if (dpsoMods & modPair.dpsoMod)
            x11Mods |= modPair.x11Mod;

    return x11Mods;
}


}
