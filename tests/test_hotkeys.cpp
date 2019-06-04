
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "dpso/dpso.h"

#include "flow.h"


static void checkHotkeyToString(const DpsoHotkey& hotkey, int line)
{
    const char* str = dpsoHotkeyToString(&hotkey);

    DpsoHotkey outHotkey;
    dpsoHotkeyFromString(str, &outHotkey);

    if (hotkey == outHotkey)
        return;

    std::fprintf(
        stderr,
        "line %i: "
        "dpsoHotkeyToString: String \"%s\" wasn't successfully parsed by "
        "dpsoHotkeyFromString())\n",
        line, str);
    test::failure();
}


const auto allMods = (
    dpsoKeyModCtrl
    | dpsoKeyModAlt
    | dpsoKeyModShift
    | dpsoKeyModWin
);


static void testHotkeyToString()
{
    #define CHECK(hotkey) checkHotkeyToString(hotkey, __LINE__)
    using HK = DpsoHotkey;

    CHECK((HK{dpsoUnknownKey, dpsoKeyModCtrl}));
    CHECK((HK{dpsoKeyY, dpsoKeyModNone}));
    CHECK((HK{dpsoKeyY, allMods}));

    #undef CHECK
}


static void checkHotkeyFromString(
    const char* str,
    const DpsoHotkey& expected,
    int lineNum)
{
    DpsoHotkey got;
    dpsoHotkeyFromString(str, &got);

    if (got == expected)
        return;

    std::fprintf(
        stderr,
        "line %i: dpsoHotkeyFromString(\"%s\"): "
        "expected (%i %i), got (%i %i)\n",
        lineNum,
        str,
        expected.key, expected.mods,
        got.key, got.mods);
    test::failure();
}


static void testHotkeyFromString()
{
    #define CHECK(str, expected) \
        checkHotkeyFromString(str, expected, __LINE__)

    using HK = DpsoHotkey;

    CHECK("Y + Ctrl", (HK{dpsoUnknownKey, dpsoKeyModNone}));
    CHECK("Ctrl Y", (HK{dpsoUnknownKey, dpsoKeyModNone}));
    CHECK("Keypad  +", (HK{dpsoUnknownKey, dpsoKeyModNone}));

    CHECK("", (HK{dpsoUnknownKey, dpsoKeyModNone}));
    CHECK("Ctrl", (HK{dpsoUnknownKey, dpsoKeyModCtrl}));
    CHECK("Y", (HK{dpsoKeyY, dpsoKeyModNone}));
    CHECK("y", (HK{dpsoKeyY, dpsoKeyModNone}));
    CHECK("  y", (HK{dpsoKeyY, dpsoKeyModNone}));
    CHECK("  y ", (HK{dpsoKeyY, dpsoKeyModNone}));
    CHECK("Keypad +", (HK{dpsoKeyKpPlus, dpsoKeyModNone}));

    CHECK("Ctrl + Alt + Shift + Windows + Y", (HK{dpsoKeyY, allMods}));
    CHECK("Alt + Shift + Ctrl + Windows + Y", (HK{dpsoKeyY, allMods}));
    CHECK("Alt+Shift+Ctrl+Windows+Y", (HK{dpsoKeyY, allMods}));
    CHECK("  CTRL + alt + shifT + WiNdows + y  ", (HK{dpsoKeyY, allMods}));

    CHECK("Ctrl+Alt+Shift+Windows+Keypad +", (HK{dpsoKeyKpPlus, allMods}));

    CHECK("Windows + Keypad +", (HK{dpsoKeyKpPlus, dpsoKeyModWin}));
    CHECK("Command + Keypad +", (HK{dpsoKeyKpPlus, dpsoKeyModWin}));
    CHECK("Super + Keypad +", (HK{dpsoKeyKpPlus, dpsoKeyModWin}));

    #undef CHECK
}


static void testHotkeys()
{
    testHotkeyToString();
    testHotkeyFromString();
}


REGISTER_TEST("hotkeys", testHotkeys);
