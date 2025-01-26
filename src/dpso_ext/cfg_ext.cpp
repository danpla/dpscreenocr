#include "cfg_ext.h"

#include <charconv>
#include <cstring>
#include <ctime>
#include <string>

#include "dpso_utils/str.h"
#include "dpso_utils/strftime.h"


static void enableLang(DpsoOcr* ocr, const char* langCode)
{
    dpsoOcrSetLangIsActive(
        ocr, dpsoOcrGetLangIdx(ocr, langCode), true);
}


const auto langSeparator = ',';


void dpsoCfgLoadActiveLangs(
    const DpsoCfg* cfg,
    const char* key,
    DpsoOcr* ocr,
    const char* fallbackLangCode)
{
    for (int i = 0; i < dpsoOcrGetNumLangs(ocr); ++i)
        dpsoOcrSetLangIsActive(ocr, i, false);

    const auto* s = dpsoCfgGetStr(cfg, key, nullptr);

    if (!s) {
        enableLang(ocr, fallbackLangCode);
        return;
    }

    std::string langCode;
    while (*s) {
        if (dpso::str::isBlank(*s) || *s == langSeparator) {
            ++s;
            continue;
        }

        const auto* langCodeBegin = s;
        const auto* langCodeEnd = s;

        for (; *s && *s != langSeparator; ++s)
            if (!dpso::str::isBlank(*s))
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
    else
        *hotkey = defaultHotkey ? *defaultHotkey : dpsoEmptyHotkey;
}


void dpsoCfgSetHotkey(
    DpsoCfg* cfg, const char* key, const DpsoHotkey* hotkey)
{
    if (hotkey)
        dpsoCfgSetStr(cfg, key, dpsoHotkeyToString(hotkey));
}


bool dpsoCfgGetTime(
    const DpsoCfg* cfg, const char* key, struct tm* time)
{
    if (!time)
        return false;

    const auto* str = dpsoCfgGetStr(cfg, key, nullptr);
    if (!str)
        return false;

    const auto* strEnd = str + std::strlen(str);

    const auto parse = [&](int& result, int min, int max)
    {
        const auto [ptr, ec] = std::from_chars(str, strEnd, result);
        if (ec != std::errc{} || result < min || result > max)
            return false;

        str = ptr;
        return true;
    };

    const auto consume = [&](char c)
    {
        if (str < strEnd && *str == c) {
            ++str;
            return true;
        }

        return false;
    };

    std::tm result{};

    if (parse(result.tm_year, 0, 99999)
            && consume('-')
            && parse(result.tm_mon, 1, 12)
            && consume('-')
            && parse(result.tm_mday, 1, 31)
            && consume(' ')
            && parse(result.tm_hour, 0, 23)
            && consume(':')
            && parse(result.tm_min, 0, 59)
            && consume(':')
            && parse(result.tm_sec, 0, 60)
            && str == strEnd){
        result.tm_year -= 1900;
        result.tm_mon -= 1;

        *time = result;
        return true;
    }

    return false;
}


void dpsoCfgSetTime(
    DpsoCfg* cfg, const char* key, const struct tm* time)
{
    if (!time)
        return;

    dpsoCfgSetStr(
        cfg, key, dpso::strftime("%Y-%m-%d %H:%M:%S", time).c_str());
}
