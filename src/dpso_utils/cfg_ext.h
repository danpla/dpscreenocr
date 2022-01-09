
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


struct DpsoCfg;
struct DpsoOcr;


/**
 * Load currently active languages.
 *
 * All languages not listed in the key's value are disabled.
 *
 * If the key doesn't exist, the language with fallbackLangCode is
 * enabled, if available. Pass an empty string as fallbackLangCode if
 * you don't want to use a fallback.
 */
void dpsoCfgLoadActiveLangs(
    const struct DpsoCfg* cfg,
    const char* key,
    struct DpsoOcr* ocr,
    const char* fallbackLangCode);

void dpsoCfgSaveActiveLangs(
    struct DpsoCfg* cfg, const char* key, const struct DpsoOcr* ocr);


struct DpsoHotkey;

/**
 * Get hotkey.
 *
 * If key exists, the hotkey is set to the result of
 * dpsoHotkeyFromString().
 *
 * If the key does not exist, defaultHotkey is returned. If
 * defaultHotkey is null, {dpsoUnknownKey, dpsoKeyModNone} is used.
 */
void dpsoCfgGetHotkey(
    const struct DpsoCfg* cfg,
    const char* key,
    struct DpsoHotkey* hotkey,
    const struct DpsoHotkey* defaultHotkey);

/**
 * Set hotkey.
 *
 * The value is set to the result of dpsoHotkeyToString().
 */
void dpsoCfgSetHotkey(
    struct DpsoCfg* cfg,
    const char* key,
    const struct DpsoHotkey* hotkey);


#ifdef __cplusplus
}
#endif
