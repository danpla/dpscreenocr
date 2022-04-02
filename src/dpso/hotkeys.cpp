
#include "hotkeys.h"

#include <cctype>
#include <string>

#include "backend/backend.h"
#include "backend/key_manager.h"
#include "key_names.h"
#include "str.h"


static dpso::backend::KeyManager* keyManager;


int dpsoGetHotkeysEnabled(void)
{
    return keyManager ? keyManager->getHotkeysEnabled() : false;
}


void dpsoSetHotheysEnabled(int newHotkeysEnabled)
{
    if (keyManager)
        keyManager->setHotkeysEnabled(newHotkeysEnabled);
}


DpsoHotkeyAction dpsoGetLastHotkeyAction(void)
{
    return keyManager ? keyManager->getLastHotkeyAction() : -1;
}


int dpsoBindHotkey(
    const struct DpsoHotkey* hotkey, DpsoHotkeyAction action)
{
    if (!keyManager
            || !hotkey
            || hotkey->key < 0
            || hotkey->key >= dpsoNumKeys
            || action < 0)
        return false;

    return keyManager->bindHotkey(*hotkey, action);
}


void dpsoUnbindHotkey(const struct DpsoHotkey* hotkey)
{
    if (!keyManager || !hotkey)
        return;

    for (int i = 0; i < keyManager->getNumBindings(); ++i)
        if (keyManager->getBinding(i).hotkey == *hotkey) {
            keyManager->removeBinding(i);
            break;
        }
}


void dpsoUnbindAction(DpsoHotkeyAction action)
{
    if (!keyManager)
        return;

    for (int i = 0; i < keyManager->getNumBindings();)
        if (keyManager->getBinding(i).action == action)
            keyManager->removeBinding(i);
        else
            ++i;
}


void dpsoFindActionHotkey(
    DpsoHotkeyAction action, struct DpsoHotkey* hotkey)
{
    if (!hotkey)
        return;

    *hotkey = {dpsoUnknownKey, dpsoKeyModNone};

    if (!keyManager)
        return;

    for (int i = 0; i < keyManager->getNumBindings(); ++i) {
        const auto& binding = keyManager->getBinding(i);
        if (binding.action == action) {
            *hotkey = binding.hotkey;
            return;
        }
    }
}


DpsoHotkeyAction dpsoFindHotkeyAction(const struct DpsoHotkey* hotkey)
{
    if (!keyManager || !hotkey)
        return -1;

    for (int i = 0; i < keyManager->getNumBindings(); ++i) {
        const auto& binding = keyManager->getBinding(i);
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

    *hotkey = {dpsoUnknownKey, dpsoKeyModNone};

    for (const auto* s = str; *s;) {
        while (std::isspace(*s))
            ++s;

        const auto* nameBegin = s;
        const auto* nameEnd = s;
        for (; *s && *s != '+'; ++s)
            if (!std::isspace(*s))
                nameEnd = s + 1;

        const auto mod = dpso::modFromString(
            nameBegin, nameEnd - nameBegin);
        if (mod != dpsoKeyModNone) {
            hotkey->mods |= mod;

            if (*s == '+')
                ++s;

            continue;
        }

        // The current substring is not a valid modifier, so
        // consume the rest of the string and assume it's a key.
        for (; *s; ++s)
            if (!std::isspace(*s))
                nameEnd = s + 1;

        hotkey->key = dpso::keyFromString(
            nameBegin, nameEnd - nameBegin);
        break;
    }
}


namespace dpso {
namespace hotkeys {


void init(dpso::backend::Backend& backend)
{
    keyManager = &backend.getKeyManager();
}


void shutdown()
{
    keyManager = nullptr;
}


}
}
