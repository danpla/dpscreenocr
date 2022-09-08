
#pragma once

#include "key_types.h"


namespace dpso {
namespace backend {


struct HotkeyBinding {
    DpsoHotkey hotkey;
    DpsoHotkeyAction action;
};


/**
 * Key manager.
 *
 * See hotkeys.h for more information.
 *
 * Preconditions:
 *
 *   * bindHotkey():
 *       * hotkey.key >= 0 and < dpsoNumKeys
 *       * action >= 0
 *   * getBinding(), removeBinding():
 *       * idx >= 0 and < getNumBindings()
 */
class KeyManager {
public:
    virtual ~KeyManager() = default;

    /**
     * Get whether hotkeys are enabled.
     *
     * \sa dpsoGetHotkeysEnabled()
     */
    virtual bool getHotkeysEnabled() const = 0;

    /**
     * Set whether hotkeys are enabled.
     *
     * \sa dpsoSetHotheysEnabled()
     */
    virtual void setHotkeysEnabled(bool newHotkeysEnabled) = 0;

    /**
     * Get action of the pressed hotkey.
     *
     * \sa dpsoGetLastHotkeyAction()
     */
    virtual DpsoHotkeyAction getLastHotkeyAction() const = 0;

    /**
     * Bind hotkey to action.
     *
     * \sa dpsoBindHotkey()
     */
    virtual bool bindHotkey(
        const DpsoHotkey& hotkey, DpsoHotkeyAction action) = 0;

    virtual int getNumBindings() const = 0;
    virtual HotkeyBinding getBinding(int idx) const = 0;
    virtual void removeBinding(int idx) = 0;
};


}
}
