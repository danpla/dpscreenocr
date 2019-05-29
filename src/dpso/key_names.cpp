
#include "key_names.h"

#include "str.h"


namespace dpso {


const char* const keyNames[dpsoNumKeys] = {
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",

    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",

    "Return",
    "Escape",
    "Backspace",
    "Tab",
    "Space",

    "-",
    "=",
    "[",
    "]",

    ";",
    "'",
    "`",
    ",",
    ".",
    "/",

    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",

    "Print Screen",
    "Pause",
    "Insert",
    "Home",
    "Page Up",
    "Delete",
    "End",
    "Page Down",
    "Right",
    "Left",
    "Down",
    "Up",

    "Keypad /",
    "Keypad *",
    "Keypad -",
    "Keypad +",
    "Keypad Enter",
    "Keypad 1",
    "Keypad 2",
    "Keypad 3",
    "Keypad 4",
    "Keypad 5",
    "Keypad 6",
    "Keypad 7",
    "Keypad 8",
    "Keypad 9",
    "Keypad 0",
    "Keypad .",
};


const char* keyToString(DpsoKey key)
{
    if (key < 0 || key >= dpsoNumKeys)
        return "";

    return keyNames[key];
}


DpsoKey keyFromString(const char* str, std::size_t strLen)
{
    for (int i = 0; i < dpsoNumKeys; ++i)
        if (str::cmpSubStr(keyNames[i], str, strLen, true) == 0)
            return static_cast<DpsoKey>(i);

    return dpsoUnknownKey;
}


namespace {


enum KeyModWinName {
    keyModWinNameWindows,
    keyModWinNameCommand,
    keyModWinNameSuper,
    numKeyModWinNames
};


const auto nativeKeyModWinName =
    #ifdef __APPLE__
    keyModWinNameCommand;
    #elif defined(__unix__)
    keyModWinNameSuper;
    #else
    keyModWinNameWindows;
    #endif


const char* const keyModWinNames[numKeyModWinNames + 1] = {
    "Windows",
    "Command",
    "Super",
    nullptr
};


enum KeyModAltName {
    keyModAltNameAlt,
    keyModAltNameOption,
    numKeyModAltNames
};


const auto nativeKeyModAltName =
    #ifdef __APPLE__
    keyModAltNameOption;
    #else
    keyModAltNameAlt;
    #endif


const char* const keyModAltNames[numKeyModAltNames + 1] = {
    "Alt",
    "Option",
    nullptr
};


const char* const keyModCtrlNames[] = {"Ctrl", nullptr};
const char* const keyModShiftNames[] = {"Shift", nullptr};


struct ModName {
    const char* const* names;
    std::size_t nativeNameIdx;
    DpsoKeyMod mod;
};


const ModName modNames[] = {
    {keyModCtrlNames, 0, dpsoKeyModCtrl},
    {keyModAltNames, nativeKeyModAltName, dpsoKeyModAlt},
    {keyModShiftNames, 0, dpsoKeyModShift},
    {keyModWinNames, nativeKeyModWinName, dpsoKeyModWin}
};


}


const char* modToString(DpsoKeyMod mod)
{
    for (const auto& modName : modNames)
        if (mod == modName.mod)
            return modName.names[modName.nativeNameIdx];

    return "";
}


DpsoKeyMod modFromString(const char* str, std::size_t strLen)
{
    for (const auto& modName : modNames)
        for (const auto* const* names = modName.names; *names; ++names)
            if (str::cmpSubStr(*names, str, strLen, true) == 0)
                return modName.mod;

    return dpsoKeyModNone;
}


}
