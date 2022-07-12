
#include "cfg_ext.h"

#include <cctype>
#include <cstring>
#include <string>

#include "dpso/str.h"


static void disableAllLangs(DpsoOcr* ocr)
{
    for (int i = 0; i < dpsoOcrGetNumLangs(ocr); ++i)
        dpsoOcrSetLangIsActive(ocr, i, false);
}


static void enableLang(DpsoOcr* ocr, const char* langCode)
{
    dpsoOcrSetLangIsActive(
        ocr, dpsoOcrGetLangIdx(ocr, langCode), true);
}


const char langSeparator = ',';


void dpsoCfgLoadActiveLangs(
    const DpsoCfg* cfg,
    const char* key,
    DpsoOcr* ocr,
    const char* fallbackLangCode)
{
    disableAllLangs(ocr);

    const auto* s = dpsoCfgGetStr(cfg, key, nullptr);

    if (!s) {
        enableLang(ocr, fallbackLangCode);
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

        langCode.assign(langCodeBegin, langCodeEnd);
        enableLang(ocr, langCode.c_str());
    }
}


void dpsoCfgSaveActiveLangs(
    DpsoCfg* cfg, const char* key, const DpsoOcr* ocr)
{
    std::string str;

    for (int i = 0; i < dpsoOcrGetNumLangs(ocr); ++i) {
        if (!dpsoOcrGetLangIsActive(ocr, i))
            continue;

        if (!str.empty()) {
            str += langSeparator;
            str += ' ';
        }

        str += dpsoOcrGetLangCode(ocr, i);
    }

    dpsoCfgSetStr(cfg, key, str.c_str());
}


void dpsoCfgGetHotkey(
    const DpsoCfg* cfg,
    const char* key,
    DpsoHotkey* hotkey,
    const DpsoHotkey* defaultHotkey)
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
    DpsoCfg* cfg, const char* key, const DpsoHotkey* hotkey)
{
    if (!hotkey)
        return;

    dpsoCfgSetStr(cfg, key, dpsoHotkeyToString(hotkey));
}
