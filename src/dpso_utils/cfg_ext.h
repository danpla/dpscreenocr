
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Load currently active languages.
 *
 * All languages not listed in the string are disabled.
 */
void dpsoCfgLoadActiveLangs(const char* key);
void dpsoCfgSaveActiveLangs(const char* key);


struct DpsoHotkey;

/**
 * Get hotkey.
 *
 * If the key does not exist, defaultHotkey is returned.
 *
 * If the string is empty, the hotkey will be {dpsoUnknownKey,
 * dpsoKeyModNone}. Otherwise, the hotkey is set to the result of
 * dpsoHotkeyFromString(). If the key of the result is dpsoUnknownKey,
 * the hotkey is set to defaultHotkey.
 *
 * Note that a string that only consists of modifiers and doesn't
 * contain a key will always result in defaultHotkey. This mirrors
 * the behavior of the routines in hotkeys.h, where a hotkey with
 * dpsoUnknownKey can be neither bound nor returned.
 */
void dpsoCfgGetHotkey(
    const char* key,
    struct DpsoHotkey* hotkey,
    const struct DpsoHotkey* defaultHotkey);

/**
 * Set hotkey.
 *
 * If the key of the hotkey is not dpsoUnknownKey, the value is set to
 * the string returned by dpsoHotkeyToString(). Otherwise, an empty
 * string is set.
 *
 * Note that it's not possible to save or load hotkeys consisting only
 * of modifiers (see dpsoCfgGetHotkey() for more details).
 */
void dpsoCfgSetHotkey(
    const char* key,
    const struct DpsoHotkey* hotkey);


#ifdef __cplusplus
}
#endif
