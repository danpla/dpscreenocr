#include "key_manager.h"

#include "backend/key_manager.h"
#include "dpso_sys_p.h"


bool dpsoKeyManagerGetIsEnabled(const DpsoKeyManager* keyManager)
{
    return keyManager && keyManager->impl.getIsEnabled();
}


void dpsoKeyManagerSetIsEnabled(
    DpsoKeyManager* keyManager, bool newIsEnabled)
{
    if (keyManager)
        keyManager->impl.setIsEnabled(newIsEnabled);
}


DpsoHotkeyAction dpsoKeyManagerGetLastHotkeyAction(
    const DpsoKeyManager* keyManager)
{
    return keyManager
        ? keyManager->impl.getLastHotkeyAction() : dpsoNoHotkeyAction;
}


void dpsoKeyManagerBindHotkey(
    DpsoKeyManager* keyManager,
    const DpsoHotkey* hotkey,
    DpsoHotkeyAction action)
{
    if (keyManager
            && hotkey
            && hotkey->key >= 0
            && hotkey->key < dpsoNumKeys
            && action >= 0)
        keyManager->impl.bindHotkey(*hotkey, action);
}


void dpsoKeyManagerUnbindHotkey(
    DpsoKeyManager* keyManager, const DpsoHotkey* hotkey)
{
    if (!keyManager || !hotkey)
        return;

    auto& impl = keyManager->impl;

    for (int i = 0; i < impl.getNumBindings(); ++i)
        if (impl.getBinding(i).hotkey == *hotkey) {
            impl.removeBinding(i);
            break;
        }
}


void dpsoKeyManagerUnbindAction(
    DpsoKeyManager* keyManager, DpsoHotkeyAction action)
{
    if (!keyManager)
        return;

    auto& impl = keyManager->impl;

    for (int i = 0; i < impl.getNumBindings();)
        if (impl.getBinding(i).action == action)
            impl.removeBinding(i);
        else
            ++i;
}


void dpsoKeyManagerFindActionHotkey(
    const DpsoKeyManager* keyManager,
    DpsoHotkeyAction action,
    DpsoHotkey* hotkey)
{
    if (!hotkey)
        return;

    *hotkey = dpsoEmptyHotkey;

    if (!keyManager)
        return;

    for (int i = 0; i < keyManager->impl.getNumBindings(); ++i) {
        const auto& binding = keyManager->impl.getBinding(i);
        if (binding.action == action) {
            *hotkey = binding.hotkey;
            return;
        }
    }
}


DpsoHotkeyAction dpsoKeyManagerFindHotkeyAction(
    const DpsoKeyManager* keyManager, const DpsoHotkey* hotkey)
{
    if (!keyManager || !hotkey)
        return dpsoNoHotkeyAction;

    for (int i = 0; i < keyManager->impl.getNumBindings(); ++i) {
        const auto& binding = keyManager->impl.getBinding(i);
        if (binding.hotkey == *hotkey)
            return binding.action;
    }

    return dpsoNoHotkeyAction;
}
