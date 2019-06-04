
#include "hotkeys.h"

#include <cctype>
#include <string>

#include "backend/backend.h"
#include "key_names.h"
#include "str.h"


static inline dpso::backend::KeyManager& getKeyManager()
{
    return dpso::backend::getBackend().getKeyManager();
}


int dpsoGetHotkeysEnabled(void)
{
    return getKeyManager().getHotkeysEnabled();
}


void dpsoSetHotheysEnabled(int newHotkeysEnabled)
{
    getKeyManager().setHotkeysEnabled(newHotkeysEnabled);
}


DpsoHotkeyAction dpsoGetLastHotkeyAction(void)
{
    return getKeyManager().getLastHotkeyAction();
}


int dpsoBindHotkey(
    const struct DpsoHotkey* hotkey, DpsoHotkeyAction action)
{
    if (!hotkey || hotkey->key < 0 || hotkey->key >= dpsoNumKeys
            || action < 0)
        return false;

    return getKeyManager().bindHotkey(*hotkey, action);
}


void dpsoUnbindHotkey(const struct DpsoHotkey* hotkey)
{
    if (!hotkey)
        return;

    for (int i = 0; i < getKeyManager().getNumBindings(); ++i) {
        dpso::backend::HotkeyBinding binding;
        getKeyManager().getBinding(i, binding);
        if (binding.hotkey == *hotkey) {
            getKeyManager().removeBinding(i);
            break;
        }
    }
}


void dpsoUnbindAction(DpsoHotkeyAction action)
{
    int i = 0;
    int numBindings = getKeyManager().getNumBindings();
    while (i < numBindings) {
        dpso::backend::HotkeyBinding binding;
        getKeyManager().getBinding(i, binding);

        if (binding.action == action) {
            getKeyManager().removeBinding(i);
            --numBindings;
        } else
            ++i;
    }
}


void dpsoFindActionHotkey(
    DpsoHotkeyAction action, struct DpsoHotkey* hotkey)
{
    if (!hotkey)
        return;

    for (int i = 0; i < getKeyManager().getNumBindings(); ++i) {
        dpso::backend::HotkeyBinding binding;
        getKeyManager().getBinding(i, binding);
        if (binding.action == action) {
            *hotkey = binding.hotkey;
            return;
        }
    }

    *hotkey = {dpsoUnknownKey, dpsoKeyModNone};
}


DpsoHotkeyAction dpsoFindHotkeyAction(const struct DpsoHotkey* hotkey)
{
    if (!hotkey)
        return -1;

    for (int i = 0; i < getKeyManager().getNumBindings(); ++i) {
        dpso::backend::HotkeyBinding binding;
        getKeyManager().getBinding(i, binding);
        if (binding.hotkey == *hotkey)
            return binding.action;
    }

    return -1;
}


DpsoKeyMod dpsoGetKeyModAt(int idx)
{
    static const DpsoKeyMod keyModsOrder[dpsoNumKeyMods] = {
        // Apple order (Control, Option, Shift, Command) matches
        // our enum:
        // https://developer.apple.com/design/human-interface-guidelines/macos/user-interaction/keyboard/
        //
        // Design guides for other platforms and desktop environments
        // don't seem to mention the order of modifiers.
        dpsoKeyModCtrl, dpsoKeyModAlt, dpsoKeyModShift, dpsoKeyModWin
    };

    if (idx < 0 || idx >= dpsoNumKeyMods)
        return dpsoKeyModNone;

    return keyModsOrder[idx];
}


const char* dpsoHotkeyToString(const struct DpsoHotkey* hotkey)
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

            str += dpso::modToString(mod);
        }
    }

    const auto* keyName = dpso::keyToString(hotkey->key);
    if (*keyName) {
        if (!str.empty())
            str += " + ";

        str += keyName;
    }

    return str.c_str();
}


void dpsoHotkeyFromString(const char* str, struct DpsoHotkey* hotkey)
{
    if (!hotkey)
        return;

    hotkey->mods = dpsoKeyModNone;
    hotkey->key = dpsoUnknownKey;

    const auto* s = str;

    while (true) {
        while (std::isspace(*s))
            ++s;

        if (!*s)
            break;

        const auto* nameEnd = s;
        const auto* end = s;
        while (*end && *end != '+') {
            if (!std::isspace(*end))
                nameEnd = end + 1;

            ++end;
        }

        const auto mod = dpso::modFromString(s, nameEnd - s);
        if (mod != dpsoKeyModNone) {
            hotkey->mods |= mod;

            s = end;

            if (*s == '+')
                ++s;

            continue;
        }

        // The current substring is not a valid modifier, so
        // consume the rest of the string and assume it's a key.
        while (*end) {
            if (!std::isspace(*end))
                nameEnd = end + 1;

            ++end;
        }

        hotkey->key = dpso::keyFromString(s, nameEnd - s);
        break;
    }
}
