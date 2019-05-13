
/* This header is a part of the C interface. */

#pragma once


/**
 * Keys.
 *
 * Note that the enum doesn't include modifiers from DpsoKeyMod
 * and Lock keys (Scroll, Num, and Caps).
 */
typedef enum {
    dpsoUnknownKey = -1,

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

    dpsoKeyReturn,
    dpsoKeyEscape,
    dpsoKeyBackspace,
    dpsoKeyTab,
    dpsoKeySpace,

    dpsoKeyMinus,
    dpsoKeyEquals,
    dpsoKeyLeftBracket,
    dpsoKeyRightBracket,

    dpsoKeySemicolon,
    dpsoKeyApostrophe,
    dpsoKeyGrave,
    dpsoKeyComma,
    dpsoKeyPeriod,
    dpsoKeySlash,

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

    dpsoKeyPrintScreen,
    dpsoKeyPause,
    dpsoKeyInsert,
    dpsoKeyHome,
    dpsoKeyPageUp,
    dpsoKeyDelete,
    dpsoKeyEnd,
    dpsoKeyPageDown,
    dpsoKeyRight,
    dpsoKeyLeft,
    dpsoKeyDown,
    dpsoKeyUp,

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
    dpsoKeyModNone = 0,
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


struct DpsoHotkey {
    /**
     * Key.
     */
    DpsoKey key;

    /**
     * Key modifiers.
     *
     * A combination of DpsoKeyMod flags.
     */
    int mods;
};


/**
 * Hotkey action.
 *
 * Hotkey action is an integral action id you associate with a hotkey
 * on dpsoBindHotkey() call. Actions >= 0 are for the user. -1 is
 * used to report various conditions, like "no hotkey was pressed"
 * for dpsoGetLastHotkeyAction(), or "hotkey is not bound" for
 * dpsoFindHotkeyAction().
 *
 * Using values < 0 in routines that expect DpsoHotkeyAction will
 * have no effect. For example, passing -1 to dpsoBindHotkey() will
 * not unbind a hotkey; you have to use dpsoUnbindHotkey() or
 * dpsoUnbindAction() instead.
 */
typedef int DpsoHotkeyAction;
