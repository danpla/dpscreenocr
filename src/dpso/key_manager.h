
#pragma once

#include <stdbool.h>

#include "keys.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get whether key manager is enabled.
 *
 * Enabling and disabling the key manager only affects
 * dpsoKeyManagerGetLastHotkeyAction(), which will always return
 * dpsoNoHotkeyAction if disabled.
 *
 * The key manager is disabled by default.
 */
bool dpsoKeyManagerGetIsEnabled(void);


/**
 * Set whether key manager is enabled.
 */
void dpsoKeyManagerSetIsEnabled(bool newIsEnabled);


/**
 * Get action of the pressed hotkey.
 *
 * dpsoKeyManagerGetLastHotkeyAction() returns the action if a bound
 * hotkey that matches the recently pressed key combination. The
 * function returns dpsoNoHotkeyAction if the key manager is disabled,
 * no keys are pressed, or there is no hotkey for the pressed
 * combination.
 *
 * It's safe to call dpsoKeyManagerGetLastHotkeyAction() several times
 * per update, since the input event handling is done in dpsoUpdate().
 */
DpsoHotkeyAction dpsoKeyManagerGetLastHotkeyAction(void);


/**
 * Bind hotkey to action.
 *
 * If the hotkey is already bound, its action will be updated.
 */
void dpsoKeyManagerBindHotkey(
    const DpsoHotkey* hotkey, DpsoHotkeyAction action);


/**
 * Unbind hotkey.
 */
void dpsoKeyManagerUnbindHotkey(const DpsoHotkey* hotkey);


/**
 * Remove all hotkeys bound to this action.
 */
void dpsoKeyManagerUnbindAction(DpsoHotkeyAction action);


/**
 * Find hotkey bound to action.
 *
 * If no hotkey is bound to the action, the function returns
 * {dpsoNoKey, dpsoNoKeyMods} (dpsoEmptyHotkey).
 *
 * If multiple hotkeys are bound to the same action, which one is
 * returned depends on the implementation.
 */
void dpsoKeyManagerFindActionHotkey(
    DpsoHotkeyAction action, DpsoHotkey* hotkey);


/**
 * Find action of hotkey.
 *
 * Returns dpsoNoHotkeyAction if the hotkey is not bound.
 */
DpsoHotkeyAction dpsoKeyManagerFindHotkeyAction(
    const DpsoHotkey* hotkey);


#ifdef __cplusplus
}
#endif
