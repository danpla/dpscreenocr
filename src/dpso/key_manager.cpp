
#include "key_manager.h"

#include "backend/backend.h"
#include "backend/key_manager.h"


static dpso::backend::KeyManager* keyManager;


bool dpsoGetHotkeysEnabled(void)
{
    return keyManager ? keyManager->getHotkeysEnabled() : false;
}


void dpsoSetHotkeysEnabled(bool newHotkeysEnabled)
{
    if (keyManager)
        keyManager->setHotkeysEnabled(newHotkeysEnabled);
}


DpsoHotkeyAction dpsoGetLastHotkeyAction(void)
{
    return keyManager
        ? keyManager->getLastHotkeyAction() : dpsoNoHotkeyAction;
}


bool dpsoBindHotkey(const DpsoHotkey* hotkey, DpsoHotkeyAction action)
{
    return
        keyManager
        && hotkey
        && hotkey->key >= 0
        && hotkey->key < dpsoNumKeys
        && action >= 0
        && keyManager->bindHotkey(*hotkey, action);
}


void dpsoUnbindHotkey(const DpsoHotkey* hotkey)
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


void dpsoFindActionHotkey(DpsoHotkeyAction action, DpsoHotkey* hotkey)
{
    if (!hotkey)
        return;

    *hotkey = dpsoEmptyHotkey;

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


DpsoHotkeyAction dpsoFindHotkeyAction(const DpsoHotkey* hotkey)
{
    if (!keyManager || !hotkey)
        return dpsoNoHotkeyAction;

    for (int i = 0; i < keyManager->getNumBindings(); ++i) {
        const auto& binding = keyManager->getBinding(i);
        if (binding.hotkey == *hotkey)
            return binding.action;
    }

    return dpsoNoHotkeyAction;
}


namespace dpso::keyManager {


void init(dpso::backend::Backend& backend)
{
    ::keyManager = &backend.getKeyManager();
}


void shutdown()
{
    ::keyManager = nullptr;
}


}
