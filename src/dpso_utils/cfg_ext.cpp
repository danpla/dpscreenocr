
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
    dpsoSetLangIsActive(dpsoGetLangIdx(langCode), true);
}


const char langSeparator = ',';


void dpsoCfgLoadActiveLangs(
    const struct DpsoCfg* cfg,
    const char* key,
    const char* fallbackLangCode)
{
    disableAllLangs();

    const auto* s = dpsoCfgGetStr(cfg, key, nullptr);

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

        const auto* langCodeBegin = s;
        const auto* langCodeEnd = s;

        for (; *s && *s != langSeparator; ++s)
            if (!std::isspace(*s))
                langCodeEnd = s + 1;

        langCode.assign(langCodeBegin, langCodeEnd - langCodeBegin);
        enableLang(langCode.c_str());
    }
}


void dpsoCfgSaveActiveLangs(struct DpsoCfg* cfg, const char* key)
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

    dpsoCfgSetStr(cfg, key, str.c_str());
}


void dpsoCfgGetHotkey(
    const struct DpsoCfg* cfg,
    const char* key,
    struct DpsoHotkey* hotkey,
    const struct DpsoHotkey* defaultHotkey)
{
    if (!hotkey)
        return;

    if (const auto* hotkeyStr = dpsoCfgGetStr(cfg, key, nullptr))
        dpsoHotkeyFromString(hotkeyStr, hotkey);
    else {
        const DpsoHotkey noneHotkey{dpsoUnknownKey, dpsoKeyModNone};
        *hotkey = defaultHotkey ? *defaultHotkey : noneHotkey;
    }
}


void dpsoCfgSetHotkey(
    struct DpsoCfg* cfg,
    const char* key,
    const struct DpsoHotkey* hotkey)
{
    if (!hotkey)
        return;

    dpsoCfgSetStr(cfg, key, dpsoHotkeyToString(hotkey));
}
