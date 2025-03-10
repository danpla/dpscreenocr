#include "backend/windows/key_manager.h"

#include <cassert>
#include <charconv>
#include <cwchar>
#include <iterator>
#include <string>

#include "dpso_utils/str.h"
#include "dpso_utils/windows/utf.h"


namespace dpso::backend::windows {


// The RegisterHotKey() documentation says that an application must
// specify an id value in the range 0x0000 through 0xBFFF, and a DLL
// must specify a value in the range 0xC000 through 0xFFFF (the range
// returned by the GlobalAddAtom()). In other words, only a DLL can
// use string atoms; a static lib must use integer ones.
//
// Fortunately, string atom functions (GlobalAddAtom() and related)
// can transparently work with integer atoms: if the atom name starts
// with # followed by a decimal number, that number is returned as
// atom. The only thing to keep in mind that an integer atom has the
// range 0x0001 - 0xBFFF; 0x0000 is not a valid value.
//
// It's thus possible to avoid two separate code paths for a static
// library and a DLL:
//
// 1. First, we make an integer atom by packing a hotkey to the first
//    15 bits of an integer: 4 bits for modifiers, 10 bits for the
//    key, and 1 sentinel bit to avoid 0x0000, which is not a valid
//    value for an integer atom. Excluding the sentinel bit, 14 bits
//    give us up to 16384 key bindings, which is exactly the number of
//    possible string atoms (range 0xC000 - 0xFFFF).
//
// 2. Then we create an atom name by converting the integer atom to a
//    string and adding a prefix. For a static lib, the prefix is #,
//    so string atom functions will return the original integer. For a
//    DLL, the prefix can be any string (even empty one), making
//    string atom functions to return a string atom.


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


const auto* const atomNamePrefix =
    #if DPSO_DLL
    L"dpso"
    #else
    L"#"
    #endif
;

const auto atomNamePrefixLen = std::wcslen(atomNamePrefix);


static std::wstring hotkeyToAtomName(const DpsoHotkey& hotkey)
{
    return
        atomNamePrefix
        + dpso::windows::utf8ToUtf16(
            str::toStr(
                sentinelBit
                | (hotkey.key << modsBits)
                | hotkey.mods).c_str());
}


static DpsoHotkey atomNameToHotkey(const wchar_t* name)
{
    if (std::wcsncmp(name, atomNamePrefix, atomNamePrefixLen) != 0)
        return dpsoEmptyHotkey;

    const auto str = dpso::windows::utf16ToUtf8(
        name + atomNamePrefixLen);

    ATOM atom{};
    const auto [ptr, ec] = std::from_chars(
        str.data(), str.data() + str.size(), atom);
    if (ec != std::errc{}
            || ptr != str.data() + str.size()
            || atom < 1)
        return dpsoEmptyHotkey;

    return {
        static_cast<DpsoKey>((atom & keyMask) >> modsBits),
        static_cast<DpsoKeyMods>(atom & modsMask)};
}


static UINT toWinKey(DpsoKey key);
static UINT toWinMods(DpsoKeyMods mods);


static void changeHotkeyState(const DpsoHotkey& hotkey, bool enabled)
{
    const auto atomName = hotkeyToAtomName(hotkey);
    auto atom = GlobalFindAtomW(atomName.c_str());

    if (!enabled) {
        if (atom == 0)
            // The hotkey was not registered because GlobalAddAtom()
            // below failed.
            return;

        UnregisterHotKey(nullptr, atom);
        GlobalDeleteAtom(atom);
        return;
    }

    const auto winKey = toWinKey(hotkey.key);
    assert(winKey != 0);  // We already checked this in bindHotkey().
    const auto winMods = toWinMods(hotkey.mods);

    if (atom == 0)
        atom = GlobalAddAtomW(atomName.c_str());
    if (atom == 0)
        return;

    RegisterHotKey(nullptr, atom, winMods, winKey);
}


KeyManager::~KeyManager()
{
    setIsEnabled(false);  // Unregister all hotkeys.
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

    for (const auto& binding : bindings)
        changeHotkeyState(binding.hotkey, isEnabled);
}


DpsoHotkeyAction KeyManager::getLastHotkeyAction() const
{
    return hotkeyAction;
}


void KeyManager::bindHotkey(
    const DpsoHotkey& hotkey, DpsoHotkeyAction action)
{
    if (auto* existingBinding = findBinding(hotkey)) {
        existingBinding->action = action;
        return;
    }

    if (toWinKey(hotkey.key) == 0)
        // There is no virtual key for the given DpsoKey.
        return;

    bindings.push_back({hotkey, action});

    if (isEnabled)
        changeHotkeyState(hotkey, true);
}


int KeyManager::getNumBindings() const
{
    return bindings.size();
}


HotkeyBinding KeyManager::getBinding(int idx) const
{
    return bindings[idx];
}


void KeyManager::removeBinding(int idx)
{
    if (isEnabled)
        changeHotkeyState(bindings[idx].hotkey, false);

    if (idx + 1 < static_cast<int>(bindings.size()))
        bindings[idx] = bindings.back();

    bindings.pop_back();
}


void KeyManager::clearLastHotkeyAction()
{
    hotkeyAction = dpsoNoHotkeyAction;
}


void KeyManager::handleWmHotkey(const MSG& msg)
{
    assert(msg.message == WM_HOTKEY);

    if (!isEnabled)
        return;

    const auto id = static_cast<int>(msg.wParam);
    if (id < 0)
        // System-defined hotkey.
        return;

    // Note that instead of using atomNameToHotkey() we could also
    // extract the hotkey from lParam of the WM_HOTKEY message.

    // GlobalAddAtom() docs say that the maximum atom name length is
    // 255 bytes (not characters), but GlobalGetAtomName() expects
    // the buffer length in characters.
    static const auto atomNameBufLen = 255 / sizeof(wchar_t) + 1;
    wchar_t atomName[atomNameBufLen];
    if (GlobalGetAtomNameW(id, atomName, atomNameBufLen) == 0)
        return;

    const auto hotkey = atomNameToHotkey(atomName);
    if (auto* binding = findBinding(hotkey))
        hotkeyAction = binding->action;
}


HotkeyBinding* KeyManager::findBinding(const DpsoHotkey& hotkey)
{
    for (auto& binding : bindings)
        if (binding.hotkey == hotkey)
            return &binding;

    return nullptr;
}


const UINT keyToVk[]{
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


static UINT toWinKey(DpsoKey key)
{
    if (key < 0 || key >= dpsoNumKeys)
        return 0;

    return keyToVk[key];
}


static UINT toWinMods(DpsoKeyMods mods)
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
