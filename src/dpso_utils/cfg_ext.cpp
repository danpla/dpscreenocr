
#include "cfg_ext.h"

#include <cctype>
#include <cstring>
#include <string>

#include "dpso/dpso.h"
#include "dpso/str.h"
#include "cfg.h"


static void disableAllLangs()
{
    for (int i = 0; i < dpsoGetNumLangs(); ++i)
        dpsoSetLangIsActive(i, false);
}


static void enableLang(const char* langCode, std::size_t langCodeLen)
{
    for (int i = 0; i < dpsoGetNumLangs(); ++i)
        if (dpso::str::cmpSubStr(
                dpsoGetLangCode(i), langCode, langCodeLen) == 0)
            dpsoSetLangIsActive(i, true);
}


const char langsSeparator = ',';


void dpsoCfgLoadActiveLangs(const char* key)
{
    disableAllLangs();

    const auto* s = dpsoCfgGetStr(key, "");

    while (true) {
        while (std::isspace(*s))
            ++s;

        if (*s == langsSeparator) {
            ++s;
            continue;
        }

        if (!*s)
            break;

        const auto* langCodeStart = s;
        const auto* langCodeEnd = s;

        while (*s && *s != langsSeparator) {
            if (!std::isspace(*s))
                langCodeEnd = s + 1;

            ++s;
        }

        enableLang(langCodeStart, langCodeEnd - langCodeStart);

        if (*s == langsSeparator)
            ++s;
    }
}


void dpsoCfgSaveActiveLangs(const char* key)
{
    std::string str;

    for (int i = 0; i < dpsoGetNumLangs(); ++i) {
        if (!dpsoGetLangIsActive(i))
            continue;

        if (!str.empty()) {
            str += langsSeparator;
            str += ' ';
        }

        str += dpsoGetLangCode(i);
    }

    dpsoCfgSetStr(key, str.c_str());
}


void dpsoCfgGetHotkey(
    const char* key,
    struct DpsoHotkey* hotkey,
    const struct DpsoHotkey* defaultHotkey)
{
    if (!hotkey)
        return;

    const auto* hotkeyStr = dpsoCfgGetStr(key, " ");
    if (!*hotkeyStr) {
        hotkey->key = dpsoUnknownKey;
        hotkey->mods = dpsoKeyModNone;
        return;
    }

    dpsoHotkeyFromString(hotkeyStr, hotkey);
    if (hotkey->key != dpsoUnknownKey)
        return;

    if (!defaultHotkey) {
        hotkey->mods = dpsoKeyModNone;
        return;
    }

    *hotkey = *defaultHotkey;
}


void dpsoCfgSetHotkey(
    const char* key,
    const struct DpsoHotkey* hotkey)
{
    if (!hotkey)
        return;

    const char* hotkeyStr;
    if (hotkey->key == dpsoUnknownKey)
        hotkeyStr = "";
    else
        hotkeyStr = dpsoHotkeyToString(hotkey);

    dpsoCfgSetStr(key, hotkeyStr);
}
