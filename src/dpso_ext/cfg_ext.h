#pragma once

#include "cfg.h"
#include "dpso_ocr/dpso_ocr.h"
#include "dpso_sys/dpso_sys.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Load currently active languages.
 *
 * Languages not listed in the key value are disabled.
 *
 * If the key doesn't exist, the language with fallbackLangCode is
 * enabled, if available. Pass an empty string as fallbackLangCode if
 * you don't want to use a fallback.
 */
void dpsoCfgLoadActiveLangs(
    const DpsoCfg* cfg,
    const char* key,
    DpsoOcr* ocr,
    const char* fallbackLangCode);

void dpsoCfgSaveActiveLangs(
    DpsoCfg* cfg, const char* key, const DpsoOcr* ocr);


/**
 * Get hotkey.
 *
 * If the key exists, the hotkey is set to the result of
 * dpsoHotkeyFromString().
 *
 * If the key does not exist, defaultHotkey is returned. If
 * defaultHotkey is null, {dpsoNoKey, dpsoNoKeyMods} is used.
 */
void dpsoCfgGetHotkey(
    const DpsoCfg* cfg,
    const char* key,
    DpsoHotkey* hotkey,
    const DpsoHotkey* defaultHotkey);


/**
 * Set hotkey.
 *
 * The value is set to the result of dpsoHotkeyToString().
 */
void dpsoCfgSetHotkey(
    DpsoCfg* cfg, const char* key, const DpsoHotkey* hotkey);


struct tm;


/**
 * Get time.
 *
 * Returns false and leaves the tm struct unchanged if the value does
 * not represent time in the "YYYY-MM-DD HH:MM:SS" format.
 */
bool dpsoCfgGetTime(
    const DpsoCfg* cfg, const char* key, struct tm* time);


void dpsoCfgSetTime(
    DpsoCfg* cfg, const char* key, const struct tm* time);


#ifdef __cplusplus
}
#endif
