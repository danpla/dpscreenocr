
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>

#include "dpso/dpso.h"
#include "dpso_utils/cfg.h"
#include "dpso_utils/cfg_ext.h"

#include "flow.h"
#include "utils.h"


namespace {


struct BasicTypesTest {
    const char* key;

    const char* strVal;
    const char* strValDefault;

    int intVal;
    int intValDefault;

    // We use int instead of bool since this is what cfg functions
    // accept and return. It should be possible to check that an
    // arbitrary int is set as "true" or "false" and, when used as
    // defaultVal in dpsoCfgGetBool(), is normalized to 0 or 1.
    int boolVal;
    int boolValDefault;
};


}


static std::string formatPossiblyNullStr(const char* str)
{
    if (!str)
        return "nullptr";

    return '"' + test::utils::escapeStr(str) + '"';
}


static void testGetStr(
    const DpsoCfg* cfg,
    const char* key,
    const char* expectedVal,
    const char* defaultVal)
{
    const auto* gotVal = dpsoCfgGetStr(cfg, key, defaultVal);
    if ((!expectedVal && !gotVal)
            || (expectedVal
                && gotVal
                && std::strcmp(gotVal, expectedVal) == 0))
        return;

    test::failure(
        "dpsoGetStr(\"%s\", %s): expected %s, got %s\n",
        key,
        formatPossiblyNullStr(defaultVal).c_str(),
        formatPossiblyNullStr(expectedVal).c_str(),
        formatPossiblyNullStr(gotVal).c_str());
}


static void testGetInt(
    const DpsoCfg* cfg,
    const char* key,
    int expectedVal,
    int defaultVal)
{
    const auto gotVal = dpsoCfgGetInt(cfg, key, defaultVal);
    if (gotVal == expectedVal)
        return;

    test::failure(
        "dpsoGetInt(\"%s\", %i): expected %i, got %i\n",
        key,
        defaultVal,
        expectedVal,
        gotVal);
}


static void testGetBool(
    const DpsoCfg* cfg,
    const char* key,
    bool expectedVal,
    int defaultVal)
{
    // Note that gotVal is int so that we can check that defaultVal
    // is normalized to 0 or 1.
    const auto gotVal = dpsoCfgGetBool(cfg, key, defaultVal);
    if (gotVal == expectedVal)
        return;

    test::failure(
        "dpsoGetBool(\"%s\", %i): expected %i, got %i\n",
        key,
        defaultVal,
        expectedVal,
        gotVal);
}


const char* const nonexistentKey = "nonexistent_key";


const std::initializer_list<BasicTypesTest> strTests{
    {"str_empty", "", "a", 5, 5, true, true},
    {"str_leading_space", " a", "", 5, 5, true, true},
    {"str_trailing_space", "a ", "", 5, 5, true, true},
    {"str_single_space", " ", "", 5, 5, true, true},
    {"str_control_chars", "\b\f\n\r\t", "", 5, 5, true, true},
    {"str_escapes", R"(\b\f\n\r\t\\ \z)", "", 5, 5, true, true},
    {"str_normal", "foo", "", 5, 5, true, true},
    {"str_int_0", "0", "", 0, 1, false, true},
    {"str_int_5", "5", "", 5, 0, true, false},
    {"str_int_minus_5", "-5", "", -5, 0, true, false},
    {"str_int_leading_space", " 5", "", 5, 0, true, false},
    {"str_int_trailing_space", "5 ", "", 5, 0, true, false},
    {"str_int_trailing_char", "5a", "", 0, 0, true, true},
    {"str_int_trailing_space_and_char", "5 a", "", 0, 0, true, true},
    {"str_float_period", "5.5", "", 0, 0, true, true},
    {"str_float_comma", "5,5", "", 0, 0, true, true},
    {"str_bool_true", "FaLsE", "", 0, 1, false, true},
    {"str_bool_false", "TrUe", "", 1, 0, true, false},
};


const std::initializer_list<BasicTypesTest> intTests{
    {"int_0", "0", "", 0, 5, false, true},
    {"int_5", "5", "", 5, 0, true, false},
    {"int_minus_5", "-5", "", -5, 0, true, false},
};


const std::initializer_list<BasicTypesTest> boolTests{
    {"bool_true", "true", "", 1, 0, true, false},
    {"bool_false", "false", "", 0, 1, false, true},
    {"bool_true_set_as_10", "true", "", 1, 0, 10, false},
    {"bool_true_set_as_minus_10", "true", "", 1, 0, -10, false},
};


static void setBasicTypes(DpsoCfg* cfg)
{
    for (const auto& test : strTests)
        dpsoCfgSetStr(cfg, test.key, test.strVal);

    for (const auto& test : intTests)
        dpsoCfgSetInt(cfg, test.key, test.intVal);

    for (const auto& test : boolTests)
        dpsoCfgSetBool(cfg, test.key, test.boolVal);
}


static void getBasicTypes(const DpsoCfg* cfg)
{
    testGetStr(cfg, nonexistentKey, nullptr, nullptr);
    testGetStr(cfg, nonexistentKey, "default", "default");

    testGetInt(cfg, nonexistentKey, 5, 5);
    testGetInt(cfg, nonexistentKey, -5, -5);

    testGetBool(cfg, nonexistentKey, false, false);
    testGetBool(cfg, nonexistentKey, true, true);
    testGetBool(cfg, nonexistentKey, true, 5);
    testGetBool(cfg, nonexistentKey, true, -5);

    for (const auto& tests : {strTests, intTests, boolTests})
        for (const auto& test : tests) {
            testGetStr(
                cfg, test.key, test.strVal, test.strValDefault);
            testGetInt(
                cfg, test.key, test.intVal, test.intValDefault);
            testGetBool(
                cfg, test.key, test.boolVal, test.boolValDefault);
        }
}


static std::string hotkeyToStr(const DpsoHotkey& hotkey)
{
    return dpsoHotkeyToString(&hotkey);
}


static void testGetHotkey(
    const DpsoCfg* cfg,
    const char* key,
    const DpsoHotkey& expectedVal,
    const DpsoHotkey& defaultVal)
{
    DpsoHotkey gotVal;
    dpsoCfgGetHotkey(cfg, key, &gotVal, &defaultVal);

    if (gotVal == expectedVal)
        return;

    test::failure(
        "dpsoCfgGetHotkey(\"%s\", &, {%s}): "
        "expected {%s}, got {%s}\n",
        key,
        hotkeyToStr(defaultVal).c_str(),
        hotkeyToStr(expectedVal).c_str(),
        hotkeyToStr(gotVal).c_str());
}


namespace {


struct HotkeyTest {
    const char* key;
    DpsoHotkey hotkey;
};


}


const HotkeyTest hotkeyTests[] = {
    {"hotkey_none", {dpsoUnknownKey, dpsoKeyModNone}},
    {
        "hotkey_mods_only",
        {dpsoUnknownKey, dpsoKeyModShift | dpsoKeyModCtrl}
    },
    {"hotkey_key_only", {dpsoKeyA, dpsoKeyModNone}},
    {"hotkey", {dpsoKeyA, dpsoKeyModShift | dpsoKeyModCtrl}},
};


static void setHotkey(DpsoCfg* cfg)
{
    for (const auto& test : hotkeyTests)
        dpsoCfgSetHotkey(cfg, test.key, &test.hotkey);
}


static void getHotkey(const DpsoCfg* cfg)
{
    const DpsoHotkey defaultHotkey{dpsoKeyF1, dpsoKeyModWin};

    testGetHotkey(cfg, nonexistentKey, defaultHotkey, defaultHotkey);

    for (const auto& test : hotkeyTests)
        testGetHotkey(cfg, test.key, test.hotkey, defaultHotkey);
}


const char* const cfgFileName = "test_cfg_file.cfg";


static void reload(DpsoCfg* cfg)
{
    if (!dpsoCfgSave(cfg, cfgFileName))
        test::fatalError(
            "reload(): dpsoCfgSave(cfg, \"%s\") failed: %s\n",
            cfgFileName,
            dpsoGetError());

    const auto loaded = dpsoCfgLoad(cfg, cfgFileName);
    std::remove(cfgFileName);

    if (!loaded)
        test::fatalError(
            "reload(): dpsoCfgLoad(cfg, \"%s\") failed: %s\n",
            cfgFileName,
            dpsoGetError());
}


static void loadCfgData(DpsoCfg* cfg, const char* cfgData)
{
    auto* fp = std::fopen(cfgFileName, "wb");
    if (!fp)
        test::fatalError(
            "loadCfgData(): fopen(\"%s\", \"wb\") failed: %s\n",
            cfgFileName,
            std::strerror(errno));

    std::fputs(cfgData, fp);
    std::fclose(fp);

    const auto loaded = dpsoCfgLoad(cfg, cfgFileName);
    std::remove(cfgFileName);

    if (!loaded)
        test::fatalError(
            "loadCfgData(): dpsoCfgLoad(cfg, \"%s\") failed: %s\n",
            cfgFileName,
            dpsoGetError());
}


static void testValueOverridingOnLoad(DpsoCfg* cfg)
{
    const std::string key = "duplicate_key";

    loadCfgData(cfg, (key + " value1\n" + key + " value2").c_str());
    testGetStr(cfg, key.c_str(), "value2", "");
}


static void testUnescapedBackslashAtEndOfQuotedString(DpsoCfg* cfg)
{
    const std::string key =
        "unescaped_backslash_at_end_of_quoted_string";

    loadCfgData(cfg, (key + " \"a\\\"").c_str());
    testGetStr(cfg, key.c_str(), "a", "");
}


static void testCfg()
{
    dpso::CfgUPtr cfg{dpsoCfgCreate()};
    if (!cfg)
        test::fatalError(
            "dpsoCfgCreate() failed: %s\n", dpsoGetError());

    setBasicTypes(cfg.get());
    setHotkey(cfg.get());

    reload(cfg.get());

    getBasicTypes(cfg.get());
    getHotkey(cfg.get());

    testValueOverridingOnLoad(cfg.get());
    testUnescapedBackslashAtEndOfQuotedString(cfg.get());
}


REGISTER_TEST(testCfg);
