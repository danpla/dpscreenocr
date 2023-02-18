
#include "keys.h"

#include <cctype>
#include <cstddef>
#include <vector>
#include <string>

#include "dpso_utils/str.h"


const DpsoHotkey dpsoEmptyHotkey{dpsoNoKey, dpsoNoKeyMods};


DpsoKeyMod dpsoGetKeyModAt(int idx)
{
    static const DpsoKeyMod keyModsOrder[] = {
        // Apple order (Control, Option, Shift, Command) matches
        // our enum:
        // https://developer.apple.com/design/human-interface-guidelines/macos/user-interaction/keyboard/
        //
        // Design guides for other platforms and desktop environments
        // don't seem to mention the order of modifiers.
        dpsoKeyModCtrl, dpsoKeyModAlt, dpsoKeyModShift, dpsoKeyModWin
    };
    static_assert(std::size(keyModsOrder) == dpsoNumKeyMods);

    if (idx < 0 || idx >= dpsoNumKeyMods)
        return dpsoNoKeyMods;

    return keyModsOrder[idx];
}


const char* const keyNames[] = {
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

    "-",
    "=",
    "Backspace",

    "[",
    "]",
    "\\",

    ";",
    "'",
    "Return",

    ",",
    ".",
    "/",

    "Escape",
    "`",
    "Tab",

    "Space",

    "Print Screen",
    "Pause",
    "Insert",
    "Delete",
    "Home",
    "End",
    "Page Up",
    "Page Down",
    "Up",
    "Down",
    "Left",
    "Right",

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
static_assert(std::size(keyNames) == dpsoNumKeys);


static const char* keyToString(DpsoKey key)
{
    if (key < 0 || key >= dpsoNumKeys)
        return "";

    return keyNames[key];
}


static DpsoKey keyFromString(const char* str, std::size_t strLen)
{
    if (strLen == 0)
        return dpsoNoKey;

    for (int i = 0; i < dpsoNumKeys; ++i)
        if (dpso::str::cmpSubStr(
                keyNames[i],
                str,
                strLen,
                dpso::str::cmpIgnoreCase) == 0)
            return static_cast<DpsoKey>(i);

    return dpsoNoKey;
}


namespace {


enum class KeyPlatformId {
    generic,
    macos,
    unix,
    windows
};


const auto nativeKeyPlatformId =
    #ifdef __APPLE__
    KeyPlatformId::macos
    #elif defined(__unix__)
    KeyPlatformId::unix
    #elif defined(_WIN32)
    KeyPlatformId::windows
    #else
    KeyPlatformId::generic
    #endif
;



struct ModNameInfo {
    struct PlatformName {
        KeyPlatformId platformId;
        const char* name;
    };

    DpsoKeyMod mod;
    const char* genericName;
    std::vector<PlatformName> platformNames;
};


}


const ModNameInfo modNameInfos[] = {
    {dpsoKeyModCtrl, "Ctrl", {}},
    {dpsoKeyModAlt, "Alt",
        {
            {KeyPlatformId::macos, "Option"},
        }
    },
    {dpsoKeyModShift, "Shift", {}},
    {dpsoKeyModWin, "Windows",
        {
            {KeyPlatformId::macos, "Command"},
            {KeyPlatformId::unix, "Super"},
        }
    },
};
static_assert(std::size(modNameInfos) == dpsoNumKeyMods);


static const char* modToString(DpsoKeyMod mod)
{
    if (mod == dpsoNoKeyMods)
        return "";

    for (const auto& modNameInfo : modNameInfos) {
        if (mod != modNameInfo.mod)
            continue;

        for (const auto& platformName : modNameInfo.platformNames)
            if (platformName.platformId == nativeKeyPlatformId)
                return platformName.name;

        return modNameInfo.genericName;
    }

    return "";
}


static DpsoKeyMod modFromString(const char* str, std::size_t strLen)
{
    if (strLen == 0)
        return dpsoNoKeyMods;

    for (const auto& modNameInfo : modNameInfos) {
        if (dpso::str::cmpSubStr(
                modNameInfo.genericName,
                str,
                strLen,
                dpso::str::cmpIgnoreCase) == 0)
            return modNameInfo.mod;

        for (const auto& platformName : modNameInfo.platformNames)
            if (dpso::str::cmpSubStr(
                    platformName.name,
                    str,
                    strLen,
                    dpso::str::cmpIgnoreCase) == 0)
                return modNameInfo.mod;
    }

    return dpsoNoKeyMods;
}


const char* dpsoHotkeyToString(const DpsoHotkey* hotkey)
{
    static std::string str;
    str.clear();

    if (!hotkey)
        return str.c_str();

    for (int i = 0; i < dpsoNumKeyMods; ++i) {
        const auto mod = dpsoGetKeyModAt(i);
        if (hotkey->mods & mod) {
            if (!str.empty())
                str += " + ";

            str += modToString(mod);
        }
    }

    const auto* keyName = keyToString(hotkey->key);
    if (*keyName) {
        if (!str.empty())
            str += " + ";

        str += keyName;
    }

    return str.c_str();
}


void dpsoHotkeyFromString(const char* str, DpsoHotkey* hotkey)
{
    if (!hotkey)
        return;

    *hotkey = dpsoEmptyHotkey;

    // There's no empty string check in the loop condition since we
    // should clear modifiers in case of an empty name after the last
    // +, e.g. "Ctrl +".
    for (const auto* s = str; true;) {
        while (std::isblank(*s))
            ++s;

        const auto* nameBegin = s;
        const auto* nameEnd = s;
        for (; *s && *s != '+'; ++s)
            if (!std::isblank(*s))
                nameEnd = s + 1;

        const auto mod = modFromString(
            nameBegin, nameEnd - nameBegin);
        if (mod != dpsoNoKeyMods
                && !(hotkey->mods & mod)) {
            hotkey->mods |= mod;

            if (*s == '+') {
                ++s;
                continue;
            }

            break;
        }

        // The current substring is either an invalid or duplicate
        // modifier, so consume the rest and assume it's a key.
        for (; *s; ++s)
            if (!std::isblank(*s))
                nameEnd = s + 1;

        hotkey->key = keyFromString(nameBegin, nameEnd - nameBegin);

        if (hotkey->key == dpsoNoKey)
            hotkey->mods = dpsoNoKeyMods;

        break;
    }
}


const DpsoHotkeyAction dpsoNoHotkeyAction = -1;
