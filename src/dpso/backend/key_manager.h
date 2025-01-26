#pragma once

#include "keys.h"


namespace dpso::backend {


struct HotkeyBinding {
    DpsoHotkey hotkey;
    DpsoHotkeyAction action;
};


// Key manager.
//
// See key_manager.h for more information. Preconditions:
//
// * bindHotkey():
//     * hotkey.key >= 0 and < dpsoNumKeys
//     * action >= 0
// * getBinding(), removeBinding():
//     * idx >= 0 and < getNumBindings()
class KeyManager {
public:
    virtual ~KeyManager() = default;

    // See dpsoKeyManagerGetIsEnabled()
    virtual bool getIsEnabled() const = 0;

    // See dpsoKeyManagerSetIsEnabled()
    virtual void setIsEnabled(bool newIsEnabled) = 0;

    // See dpsoKeyManagerGetLastHotkeyAction()
    virtual DpsoHotkeyAction getLastHotkeyAction() const = 0;

    // See dpsoKeyManagerBindHotkey()
    virtual void bindHotkey(
        const DpsoHotkey& hotkey, DpsoHotkeyAction action) = 0;

    virtual int getNumBindings() const = 0;
    virtual HotkeyBinding getBinding(int idx) const = 0;
    virtual void removeBinding(int idx) = 0;
};


}
