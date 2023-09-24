
#include <string>

#include "dpso/dpso.h"

#include "flow.h"


namespace {


const auto allMods =
    dpsoKeyModCtrl | dpsoKeyModAlt | dpsoKeyModShift | dpsoKeyModWin;


void testHotkeyToString()
{
    const DpsoHotkey hotkeys[] = {
        {dpsoNoKey, dpsoKeyModCtrl},
        {dpsoKeyY, dpsoNoKeyMods},
        {dpsoKeyY, allMods},
    };

    for (const auto& hotkey : hotkeys) {
        const char* str = dpsoHotkeyToString(&hotkey);

        DpsoHotkey outHotkey;
        dpsoHotkeyFromString(str, &outHotkey);

        if (hotkey == outHotkey)
            continue;

        test::failure(
            "testHotkeyToString: \"%s\" wasn't successfully parsed "
            "by dpsoHotkeyFromString())\n",
            str);
    }
}


std::string toStr(const DpsoHotkey& hotkey)
{
    return dpsoHotkeyToString(&hotkey);
}


void testHotkeyFromString()
{
    const struct Test {
        const char* str;
        DpsoHotkey expectedHotkey;
    } tests[] = {
        {"", dpsoEmptyHotkey},
        {"???", dpsoEmptyHotkey},
        {"Ctrl + ???", dpsoEmptyHotkey},
        {"Ctrl +", dpsoEmptyHotkey},
        {"Ctrl + ", dpsoEmptyHotkey},
        {"Ctrl + + ", dpsoEmptyHotkey},
        {"+ Ctrl", dpsoEmptyHotkey},
        {"A", {dpsoKeyA, dpsoNoKeyMods}},
        {"Ctrl", {dpsoNoKey, dpsoKeyModCtrl}},
        {"Ctrl + A", {dpsoKeyA, dpsoKeyModCtrl}},
        {"Ctrl A", dpsoEmptyHotkey},
        {"A + Ctrl", dpsoEmptyHotkey},
        {"Ctrl + Ctrl + A", dpsoEmptyHotkey},
        {"Keypad +", {dpsoKeyKpPlus, dpsoNoKeyMods}},
        {"Keypad  +", dpsoEmptyHotkey},
        {"Keypad\n+", dpsoEmptyHotkey},
        {"Keypad\t+", dpsoEmptyHotkey},

        {"Ctrl + Alt + Shift + Windows + A", {dpsoKeyA, allMods}},
        {"Alt + Shift + Ctrl + Windows + A", {dpsoKeyA, allMods}},
        {"Alt+Shift+Ctrl+Windows+A", {dpsoKeyA, allMods}},
        {
            "  CTRL  +  alt  +  shifT  +  WiNdows  +  a  ",
            {dpsoKeyA, allMods}
        },
        {"Ctrl+Alt+Shift+Windows+Keypad +", {dpsoKeyKpPlus, allMods}},

        {"\t Ctrl\t +\t A\t ", {dpsoKeyA, dpsoKeyModCtrl}},
        {"\nA", dpsoEmptyHotkey},
        {"A\n", dpsoEmptyHotkey},
        {"Ctrl\n+\nA", dpsoEmptyHotkey},

        {"Windows", {dpsoNoKey, dpsoKeyModWin}},
        {"Command", {dpsoNoKey, dpsoKeyModWin}},
        {"Super", {dpsoNoKey, dpsoKeyModWin}},

        {"Alt", {dpsoNoKey, dpsoKeyModAlt}},
        {"Option", {dpsoNoKey, dpsoKeyModAlt}},
    };

    for (const auto& test : tests) {
        DpsoHotkey got;
        dpsoHotkeyFromString(test.str, &got);

        if (got == test.expectedHotkey)
            continue;

        test::failure(
            "testHotkeyFromString: dpsoHotkeyFromString(\"%s\"): "
            "expected \"%s\", got \"%s\"\n",
            test.str,
            toStr(test.expectedHotkey).c_str(),
            toStr(got).c_str());
    }
}


void testKeys()
{
    testHotkeyToString();
    testHotkeyFromString();
}


}


REGISTER_TEST(testKeys);
