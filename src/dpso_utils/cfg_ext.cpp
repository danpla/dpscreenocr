
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


static void enableLang(const char* langCode)
{
    const auto langIdx = dpsoGetLangIdx(langCode);
    if (langIdx != -1)
        dpsoSetLangIsActive(langIdx, true);
}


const char langSeparator = ',';


void dpsoCfgLoadActiveLangs(
    const char* key, const char* fallbackLangCode)
{
    disableAllLangs();

    const auto* s = dpsoCfgGetStr(key, nullptr);

    if (!s) {
        enableLang(fallbackLangCode);
        return;
    }

    std::string langCode;
    while (*s) {
        if (std::isspace(*s) || *s == langSeparator) {
            ++s;
            continue;
        }

        const auto* langCodeStart = s;
        const auto* langCodeEnd = s;

        while (*s && *s != langSeparator) {
            if (!std::isspace(*s))
                langCodeEnd = s + 1;

            ++s;
        }

        langCode.assign(langCodeStart, langCodeEnd - langCodeStart);
        enableLang(langCode.c_str());
    }
}


void dpsoCfgSaveActiveLangs(const char* key)
{
    std::string str;

    for (int i = 0; i < dpsoGetNumLangs(); ++i) {
        if (!dpsoGetLangIsActive(i))
            continue;

        if (!str.empty()) {
            str += langSeparator;
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

    const DpsoHotkey noneHotkey {dpsoUnknownKey, dpsoKeyModNone};
    const auto* fallbackHotkey = (
        defaultHotkey ? defaultHotkey : &noneHotkey);

    const auto* hotkeyStr = dpsoCfgGetStr(key, nullptr);

    if (!hotkeyStr)
        *hotkey = *fallbackHotkey;
    else if (!*hotkeyStr)
        *hotkey = noneHotkey;
    else {
        dpsoHotkeyFromString(hotkeyStr, hotkey);
        if (hotkey->key == dpsoUnknownKey)
            *hotkey = *fallbackHotkey;
    }
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
