
/**
 * \file
 * Hotkeys management
 */

#pragma once

#include "key_types.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get whether hotkeys are enabled.
 *
 * Enabling and disabling hotkeys only affects
 * dpsoGetLastHotkeyAction(), which will always return -1 if hotkeys
 * are disabled.
 *
 * Hotkeys are disabled by default.
 */
int dpsoGetHotkeysEnabled(void);


/**
 * Set whether hotkeys are enabled.
 *
 * \sa dpsoGetHotkeysEnabled()
 */
void dpsoSetHotheysEnabled(int newHotkeysEnabled);


/**
 * Get action of the pressed hotkey.
 *
 * dpsoGetLastHotkeyAction() returns the action if a bound hotkey
 * that matches the recently pressed key combination. The function
 * returns -1 if hotkeys are disabled (see dpsoGetHotkeysEnabled()),
 * no keys are pressed, or there is no hotkey for the pressed
 * combination.
 *
 * It's safe to call dpsoGetLastHotkeyAction() many times per update,
 * since the input event handling is done in dpsoUpdate().
 */
DpsoHotkeyAction dpsoGetLastHotkeyAction(void);


/**
 * Bind hotkey to action.
 *
 * If the hotkey is already bound, its action will be updated.
 *
 * Returns 0 if hotkey wasn't bound. Reasons include:
 *   * hotkey is null
 *   * hotkey->key is dpsoUnknownKey
 *   * hotkey->key is not supported by the backend
 *   * action is < 0
 */
int dpsoBindHotkey(
    const struct DpsoHotkey* hotkey, DpsoHotkeyAction action);


/**
 * Unbind hotkey.
 */
void dpsoUnbindHotkey(const struct DpsoHotkey* hotkey);


/**
 * Remove all hotkeys bound to this action.
 */
void dpsoUnbindAction(DpsoHotkeyAction action);


/**
 * Find hotkey bound to action.
 *
 * If no hotkey is bound to the action, the function returns
 * {dpsoUnknownKey, dpsoKeyModNone}.
 *
 * If multiple hotkeys are bound to the same action, which one is
 * returned depends on the implementation.
 */
void dpsoFindActionHotkey(
    DpsoHotkeyAction action, struct DpsoHotkey* hotkey);


/**
 * Find action of hotkey.
 *
 * Returns -1 if the hotkey is not bound.
 */
DpsoHotkeyAction dpsoFindHotkeyAction(
    const struct DpsoHotkey* hotkey);


/**
 * Get key modifiers in platform-specific order.
 *
 * Iterating from 0 to dpsoNumKeyMods will give you all key modifiers
 * in a platforms-specific order. Using an index outside the range
 * will result in dpsoKeyModNone.
 */
DpsoKeyMod dpsoGetKeyModAt(int idx);


/**
 * Convert hotkey to string.
 *
 * The function returns the string containing modifiers followed by
 * the key separated by " + ", for example, "Ctrl + Shift + F1". If
 * you need to query the name of a single modifier key, set key to
 * dpsoUnknownKey. Using dpsoUnknownKey with dpsoKeyModNone will give
 * an empty string.
 *
 * Some names depend on the platform. The name of the Windows key is
 * "Windows" on Windows, "Command" on macOS, and "Super" on Unix-like
 * systems. Alt key is "Option" on macOS. This function always
 * returns the platform-specific name, while dpsoHotkeyFromString()
 * accepts all the mentioned alternatives regardless of the current
 * platform.
 */
const char* dpsoHotkeyToString(const struct DpsoHotkey* hotkey);


/**
 * Convert string to hotkey.
 *
 * dpsoHotkeyFromString() accepts strings in the format created by
 * dpsoHotkeyToString(). The function ignores letter case, ignores
 * whitespace (leading, trailing, or around "+" separator), allows
 * any order of modifiers, and accepts all known platform-specific
 * modifier names regardless of the current platform (see
 * dpsoHotkeyToString()). The only requirement is that a key name
 * (if any) should always be after the modifiers.
 *
 * Keep in mind that a space within a key name is not ignored. For
 * example "Page Up" is a valid key name, but "PageUp" or "Page   Up"
 * is not.
 */
void dpsoHotkeyFromString(const char* str, struct DpsoHotkey* hotkey);


#ifdef __cplusplus
}
#endif
