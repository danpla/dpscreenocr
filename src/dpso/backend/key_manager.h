
#pragma once

#include "keys.h"


namespace dpso::backend {


struct HotkeyBinding {
    DpsoHotkey hotkey;
    DpsoHotkeyAction action;
};


/**
 * Key manager.
 *
 * See hotkeys.h for more information. Preconditions:
 *
 * * bindHotkey():
 *     * hotkey.key >= 0 and < dpsoNumKeys
 *     * action >= 0
 * * getBinding(), removeBinding():
 *     * idx >= 0 and < getNumBindings()
 */
class KeyManager {
public:
    virtual ~KeyManager() = default;

    /**
     * See dpsoGetHotkeysEnabled()
     */
    virtual bool getHotkeysEnabled() const = 0;

    /**
     * See dpsoSetHotkeysEnabled()
     */
    virtual void setHotkeysEnabled(bool newHotkeysEnabled) = 0;

    /**
     * See dpsoGetLastHotkeyAction()
     */
    virtual DpsoHotkeyAction getLastHotkeyAction() const = 0;

    /**
     * See dpsoBindHotkey()
     */
    virtual bool bindHotkey(
        const DpsoHotkey& hotkey, DpsoHotkeyAction action) = 0;

    virtual int getNumBindings() const = 0;
    virtual HotkeyBinding getBinding(int idx) const = 0;
    virtual void removeBinding(int idx) = 0;
};


}
