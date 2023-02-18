
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Keys.
 *
 * Note that the enum doesn't include modifiers from DpsoKeyMod
 * and Lock keys (Scroll, Num, and Caps).
 */
typedef enum {
    dpsoNoKey = -1,

    dpsoKeyF1,
    dpsoKeyF2,
    dpsoKeyF3,
    dpsoKeyF4,
    dpsoKeyF5,
    dpsoKeyF6,
    dpsoKeyF7,
    dpsoKeyF8,
    dpsoKeyF9,
    dpsoKeyF10,
    dpsoKeyF11,
    dpsoKeyF12,

    dpsoKeyA,
    dpsoKeyB,
    dpsoKeyC,
    dpsoKeyD,
    dpsoKeyE,
    dpsoKeyF,
    dpsoKeyG,
    dpsoKeyH,
    dpsoKeyI,
    dpsoKeyJ,
    dpsoKeyK,
    dpsoKeyL,
    dpsoKeyM,
    dpsoKeyN,
    dpsoKeyO,
    dpsoKeyP,
    dpsoKeyQ,
    dpsoKeyR,
    dpsoKeyS,
    dpsoKeyT,
    dpsoKeyU,
    dpsoKeyV,
    dpsoKeyW,
    dpsoKeyX,
    dpsoKeyY,
    dpsoKeyZ,

    dpsoKey1,
    dpsoKey2,
    dpsoKey3,
    dpsoKey4,
    dpsoKey5,
    dpsoKey6,
    dpsoKey7,
    dpsoKey8,
    dpsoKey9,
    dpsoKey0,

    dpsoKeyMinus,
    dpsoKeyEquals,
    dpsoKeyBackspace,

    dpsoKeyLeftBracket,
    dpsoKeyRightBracket,
    dpsoKeyBackslash,

    dpsoKeySemicolon,
    dpsoKeyApostrophe,
    dpsoKeyReturn,

    dpsoKeyComma,
    dpsoKeyPeriod,
    dpsoKeySlash,

    dpsoKeyEscape,
    dpsoKeyGrave,
    dpsoKeyTab,

    dpsoKeySpace,

    dpsoKeyPrintScreen,
    dpsoKeyPause,
    dpsoKeyInsert,
    dpsoKeyDelete,
    dpsoKeyHome,
    dpsoKeyEnd,
    dpsoKeyPageUp,
    dpsoKeyPageDown,
    dpsoKeyUp,
    dpsoKeyDown,
    dpsoKeyLeft,
    dpsoKeyRight,

    dpsoKeyKpDivide,
    dpsoKeyKpMultiply,
    dpsoKeyKpMinus,
    dpsoKeyKpPlus,
    dpsoKeyKpEnter,
    dpsoKeyKp1,
    dpsoKeyKp2,
    dpsoKeyKp3,
    dpsoKeyKp4,
    dpsoKeyKp5,
    dpsoKeyKp6,
    dpsoKeyKp7,
    dpsoKeyKp8,
    dpsoKeyKp9,
    dpsoKeyKp0,
    dpsoKeyKpPeriod,

    dpsoNumKeys
} DpsoKey;


/**
 * Key modifiers.
 */
typedef enum {
    dpsoNoKeyMods = 0,

    dpsoKeyModCtrl = 1 << 0,
    dpsoKeyModAlt = 1 << 1,
    dpsoKeyModShift = 1 << 2,
    dpsoKeyModWin = 1 << 3
} DpsoKeyMod;


enum {
    /**
     * The number of key modifiers in DpsoKeyMod enum.
     */
    dpsoNumKeyMods = 4
};


/**
 * Combination of DpsoKeyMod flags.
 */
typedef unsigned DpsoKeyMods;


/**
 * Get key modifiers in platform-specific order.
 *
 * Iterating from 0 to dpsoNumKeyMods will give you all key modifiers
 * in a platforms-specific order. Using an index outside the range
 * will result in dpsoNoKeyMods.
 */
DpsoKeyMod dpsoGetKeyModAt(int idx);


typedef struct DpsoHotkey {
    DpsoKey key;
    DpsoKeyMods mods;
} DpsoHotkey;


/**
 * DpsoHotkey{dpsoNoKey, dpsoNoKeyMods}
 */
extern const DpsoHotkey dpsoEmptyHotkey;


/**
 * Convert hotkey to string.
 *
 * The function returns the string containing modifiers followed by
 * the key separated by " + ", for example, "Ctrl + Shift + F1". If
 * you need to query the name of a single modifier key, set key to
 * dpsoNoKey. Using dpsoNoKey with dpsoNoKeyMods will give an empty
 * string.
 *
 * Some names depend on the platform. The name of the Windows key is
 * "Windows" on Windows, "Command" on macOS, and "Super" on Unix-like
 * systems. Alt key is "Option" on macOS. This function always
 * returns the platform-specific name, while dpsoHotkeyFromString()
 * accepts all the mentioned alternatives regardless of the current
 * platform.
 */
const char* dpsoHotkeyToString(const DpsoHotkey* hotkey);


/**
 * Convert string to hotkey.
 *
 * dpsoHotkeyFromString() accepts strings in the format created by
 * dpsoHotkeyToString(). The function ignores letter case, ignores
 * spaces and tabs (leading, trailing, or around "+" separator),
 * allows any order of modifiers, and accepts all known
 * platform-specific modifier names regardless of the current
 * platform. The only requirement is that a key name (if any) should
 * always be after the modifiers.
 *
 * Keep in mind that multi-word key names use a single space as a word
 * separator. For example "Page Up" is a valid key name, but "PageUp"
 * or "Page   Up" is not.
 */
void dpsoHotkeyFromString(const char* str, DpsoHotkey* hotkey);


/**
 * Hotkey action.
 *
 * Hotkey action is an integral action id you associate with a hotkey
 * on dpsoBindHotkey() call. Actions >= 0 are for the user. -1
 * (dpsoNoHotkeyAction) is used to report various conditions, like
 * "no hotkey was pressed" for dpsoGetLastHotkeyAction(), or "hotkey
 * is not bound" for dpsoFindHotkeyAction().
 *
 * Routines that take DpsoHotkeyAction will do nothing if the action
 * is < 0.
 */
typedef int DpsoHotkeyAction;


/**
 * DpsoHotkeyAction with value -1.
 */
extern const DpsoHotkeyAction dpsoNoHotkeyAction;


#ifdef __cplusplus
}


inline bool operator==(const DpsoHotkey& a, const DpsoHotkey& b)
{
    return a.key == b.key && a.mods == b.mods;
}


inline bool operator!=(const DpsoHotkey& a, const DpsoHotkey& b)
{
    return !(a == b);
}


#endif

