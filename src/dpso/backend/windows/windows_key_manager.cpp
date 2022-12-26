
#include "backend/windows/windows_key_manager.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>


namespace dpso::backend {


// The RegisterHotKey() documentation says that an application must
// specify an id value in the range 0x0000 through 0xBFFF, and a
// DLL must specify a value in the range 0xC000 through 0xFFFF (the
// range returned by the GlobalAddAtom()). In other words, only a DLL
// can use string atoms; a static lib must use integer ones.
//
// Fortunately, string atom functions (GlobalAddAtom() and related)
// can transparently work with integer atoms: if the atom name starts
// with # followed by the decimal number, that number is returned as
// atom. The only thing too keep in mind that an integer atom has the
// range 0x0001 - 0xBFFF; 0x0000 is not a valid value.
//
// It thus possible to make our life simpler if we ever need to use
// this library as a DLL:
//
// 1. First, we make an integer atom by packing a hotkey to the first
//    15 bits of an integer:
//
//       * 4 bits for modifiers.
//       * 10 bits for the key. Actually, 9 is more than enough.
//       * 1 bit for the sentinel to avoid 0x0000, which is not a
//         valid value for an integer atom.
//
//    Excluding the sentinel bit, 14 bits give us up to 16384 key
//    bindings, which is exactly the number of possible string atoms
//    (range 0xC000 - 0xFFFF).
//
// 2. Then we convert the integer atom to a string with a prefix to be
//    used as an atom name. For a static lib, the prefix is #, so
//    string atom functions will return the original integer. For a
//    dynamic lib, the prefix can be any string (even empty one),
//    making string atom functions to return a string atom.


static constexpr unsigned createMask(unsigned numBits)
{
    return ~(~0u << numBits);
}


const auto modsBits = 4;
const auto modsMask = createMask(modsBits);
static_assert(dpsoNumKeyMods <= modsBits);

const auto keyBits = 10;
const auto keyMask = createMask(keyBits) << modsBits;
static_assert(dpsoNumKeys <= 1 << keyBits);

const auto sentinelBit = 1 << (modsBits + keyBits);

static_assert((1 << (modsBits + keyBits)) <= 0xFFFF - 0xC000 + 1);


const wchar_t atomNamePrefix[] =
    #if DPSO_DLL
    L"dpso"
    #else
    L"#"
    #endif
;

const auto atomNamePrefixLen = std::size(atomNamePrefix) - 1;


static std::wstring hotkeyToAtomName(const DpsoHotkey& hotkey)
{
    const ATOM atom =
        sentinelBit | (hotkey.key << modsBits) | hotkey.mods;

    return atomNamePrefix + std::to_wstring(atom);
}


static DpsoHotkey atomNameToHotkey(const wchar_t* name)
{
    if (std::wcsncmp(name, atomNamePrefix, atomNamePrefixLen) != 0)
        return dpsoEmptyHotkey;

    const auto atom = _wtoi(name + atomNamePrefixLen);
    if (atom < 1)
        return dpsoEmptyHotkey;

    return {
        static_cast<DpsoKey>((atom & keyMask) >> modsBits),
        static_cast<DpsoKeyMods>(atom & modsMask)
    };
}


static UINT dpsoKeyToWinKey(DpsoKey key);
static UINT dpsoModsToWinMods(DpsoKeyMods mods);


static void changeHotkeyState(const DpsoHotkey& hotkey, bool enabled)
{
    const auto atomName = hotkeyToAtomName(hotkey);
    auto atom = GlobalFindAtomW(atomName.c_str());

    if (!enabled) {
        if (atom == 0)
            // The hotkey was not registered because GlobalAddAtom()
            // failed. See below.
            return;

        UnregisterHotKey(nullptr, atom);
        GlobalDeleteAtom(atom);
        return;
    }

    const auto winKey = dpsoKeyToWinKey(hotkey.key);
    assert(winKey != 0);  // We have checked this in bindHotkey().
    const auto winMods = dpsoModsToWinMods(hotkey.mods);

    if (atom == 0)
        atom = GlobalAddAtomW(atomName.c_str());
    if (atom == 0)
        return;

    RegisterHotKey(nullptr, atom, winMods, winKey);
}


WindowsKeyManager::WindowsKeyManager()
    : bindings{}
    , hotkeysEnabled{}
    , hotkeyAction{dpsoNoHotkeyAction}
{
}


WindowsKeyManager::~WindowsKeyManager()
{
    // Make sure we unregister all hotkeys.
    setHotkeysEnabled(false);
}


bool WindowsKeyManager::getHotkeysEnabled() const
{
    return hotkeysEnabled;
}


void WindowsKeyManager::setHotkeysEnabled(bool newHotkeysEnabled)
{
    if (newHotkeysEnabled == hotkeysEnabled)
        return;

    hotkeysEnabled = newHotkeysEnabled;

    if (!hotkeysEnabled)
        hotkeyAction = dpsoNoHotkeyAction;

    for (const auto& binding : bindings)
        changeHotkeyState(binding.hotkey, hotkeysEnabled);
}


DpsoHotkeyAction WindowsKeyManager::getLastHotkeyAction() const
{
    return hotkeyAction;
}


bool WindowsKeyManager::bindHotkey(
    const DpsoHotkey& hotkey, DpsoHotkeyAction action)
{
    if (auto* existingBinding = findBinding(hotkey)) {
        existingBinding->action = action;
        return true;
    }

    if (dpsoKeyToWinKey(hotkey.key) == 0)
        // There is no virtual key for the given DpsoKey.
        return false;

    bindings.push_back({hotkey, action});

    if (hotkeysEnabled)
        changeHotkeyState(hotkey, true);

    return true;
}


int WindowsKeyManager::getNumBindings() const
{
    return bindings.size();
}


HotkeyBinding WindowsKeyManager::getBinding(int idx) const
{
    return bindings[idx];
}


void WindowsKeyManager::removeBinding(int idx)
{
    if (hotkeysEnabled)
        changeHotkeyState(bindings[idx].hotkey, false);

    if (idx + 1 < static_cast<int>(bindings.size()))
        bindings[idx] = bindings.back();

    bindings.pop_back();
}


void WindowsKeyManager::clearLastHotkeyAction()
{
    hotkeyAction = dpsoNoHotkeyAction;
}


void WindowsKeyManager::handleWmHotkey(const MSG& msg)
{
    assert(msg.message == WM_HOTKEY);

    if (!hotkeysEnabled)
        return;

    const auto id = static_cast<int>(msg.wParam);
    if (id < 0)
        // System-defined hotkey.
        return;

    // GlobalAddAtom() docs say that the maximum atom name length is
    // 255 bytes (not characters), but GlobalGetAtomName() expects
    // the buffer length in characters.
    static const auto atomNameBufLen = 255 / sizeof(wchar_t) + 1;
    static wchar_t atomName[atomNameBufLen];
    if (GlobalGetAtomNameW(id, atomName, atomNameBufLen) == 0)
        return;

    const auto hotkey = atomNameToHotkey(atomName);
    if (auto* binding = findBinding(hotkey))
        hotkeyAction = binding->action;
}


HotkeyBinding* WindowsKeyManager::findBinding(
    const DpsoHotkey& hotkey)
{
    for (auto& binding : bindings)
        if (binding.hotkey == hotkey)
            return &binding;

    return nullptr;
}


const UINT keyToVk[] = {
    VK_F1,
    VK_F2,
    VK_F3,
    VK_F4,
    VK_F5,
    VK_F6,
    VK_F7,
    VK_F8,
    VK_F9,
    VK_F10,
    VK_F11,
    VK_F12,

    'A',
    'B',
    'C',
    'D',
    'E',
    'F',
    'G',
    'H',
    'I',
    'J',
    'K',
    'L',
    'M',
    'N',
    'O',
    'P',
    'Q',
    'R',
    'S',
    'T',
    'U',
    'V',
    'W',
    'X',
    'Y',
    'Z',

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

    VK_OEM_MINUS,
    VK_OEM_PLUS,  // 0xBB, equals
    VK_BACK,

    VK_OEM_4,  // 0xDB, left bracket
    VK_OEM_6,  // 0xDD, right bracket
    VK_OEM_5,  // 0xDC, backslash

    VK_OEM_1,  // 0xBA, semicolon
    VK_OEM_7,  // 0xDE, apostrophe
    VK_RETURN,

    VK_OEM_COMMA,
    VK_OEM_PERIOD,
    VK_OEM_2,  // 0xBF, slash

    VK_ESCAPE,
    VK_OEM_3,  // 0xC0, grave
    VK_TAB,

    VK_SPACE,

    VK_PRINT,
    VK_PAUSE,
    VK_INSERT,
    VK_DELETE,
    VK_HOME,
    VK_END,
    VK_PRIOR,
    VK_NEXT,
    VK_UP,
    VK_DOWN,
    VK_LEFT,
    VK_RIGHT,

    VK_DIVIDE,
    VK_MULTIPLY,
    VK_SUBTRACT,
    VK_ADD,
    VK_RETURN,
    VK_NUMPAD1,
    VK_NUMPAD2,
    VK_NUMPAD3,
    VK_NUMPAD4,
    VK_NUMPAD5,
    VK_NUMPAD6,
    VK_NUMPAD7,
    VK_NUMPAD8,
    VK_NUMPAD9,
    VK_NUMPAD0,
    VK_DELETE,
};
static_assert(std::size(keyToVk) == dpsoNumKeys);


static UINT dpsoKeyToWinKey(DpsoKey key)
{
    if (key < 0 || key >= dpsoNumKeys)
        return 0;

    return keyToVk[key];
}


static UINT dpsoModsToWinMods(DpsoKeyMods mods)
{
    UINT winMods = MOD_NOREPEAT;

    if (mods & dpsoKeyModCtrl)
        winMods |= MOD_CONTROL;
    if (mods & dpsoKeyModAlt)
        winMods |= MOD_ALT;
    if (mods & dpsoKeyModShift)
        winMods |= MOD_SHIFT;
    if (mods & dpsoKeyModWin)
        winMods |= MOD_WIN;

    return winMods;
}


}
