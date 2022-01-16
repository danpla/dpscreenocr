
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "dpso/dpso.h"

#include "flow.h"


const auto allMods = (
    dpsoKeyModCtrl
    | dpsoKeyModAlt
    | dpsoKeyModShift
    | dpsoKeyModWin
);


static void testHotkeyToString()
{
    const DpsoHotkey hotkeys[] = {
        {dpsoUnknownKey, dpsoKeyModCtrl},
        {dpsoKeyY, dpsoKeyModNone},
        {dpsoKeyY, allMods},
    };

    for (const auto& hotkey : hotkeys) {
        const char* str = dpsoHotkeyToString(&hotkey);

        DpsoHotkey outHotkey;
        dpsoHotkeyFromString(str, &outHotkey);

        if (hotkey == outHotkey)
            continue;

        std::fprintf(
            stderr,
            "testHotkeyToString: \"%s\" wasn't successfully parsed "
            "by dpsoHotkeyFromString())\n",
            str);
        test::failure();
    }
}


static void testHotkeyFromString()
{
    struct Test {
        const char* str;
        DpsoHotkey expectedHotkey;
    };

    const Test tests[] = {
        {"Y + Ctrl", {dpsoUnknownKey, dpsoKeyModNone}},
        {"Ctrl Y", {dpsoUnknownKey, dpsoKeyModNone}},
        {"Keypad  +", {dpsoUnknownKey, dpsoKeyModNone}},

        {"", {dpsoUnknownKey, dpsoKeyModNone}},
        {"Ctrl", {dpsoUnknownKey, dpsoKeyModCtrl}},
        {"Y", {dpsoKeyY, dpsoKeyModNone}},
        {"y", {dpsoKeyY, dpsoKeyModNone}},
        {"  y", {dpsoKeyY, dpsoKeyModNone}},
        {"  y ", {dpsoKeyY, dpsoKeyModNone}},
        {"Keypad +", {dpsoKeyKpPlus, dpsoKeyModNone}},

        {"Ctrl + Alt + Shift + Windows + Y", {dpsoKeyY, allMods}},
        {"Alt + Shift + Ctrl + Windows + Y", {dpsoKeyY, allMods}},
        {"Alt+Shift+Ctrl+Windows+Y", {dpsoKeyY, allMods}},
        {"  CTRL + alt + shifT + WiNdows + y  ", {dpsoKeyY, allMods}},

        {"Ctrl+Alt+Shift+Windows+Keypad +", {dpsoKeyKpPlus, allMods}},

        {"Windows + Keypad +", {dpsoKeyKpPlus, dpsoKeyModWin}},
        {"Command + Keypad +", {dpsoKeyKpPlus, dpsoKeyModWin}},
        {"Super + Keypad +", {dpsoKeyKpPlus, dpsoKeyModWin}},
    };

    for (const auto& test : tests) {
        DpsoHotkey got;
        dpsoHotkeyFromString(test.str, &got);

        if (got == test.expectedHotkey)
            continue;

        std::fprintf(
            stderr,
            "testHotkeyFromString: dpsoHotkeyFromString(\"%s\"): "
            "expected (%i %u), got (%i %u)\n",
            test.str,
            test.expectedHotkey.key, test.expectedHotkey.mods,
            got.key, got.mods);
        test::failure();
    }
}


static void testHotkeys()
{
    testHotkeyToString();
    testHotkeyFromString();
}


REGISTER_TEST(testHotkeys);
