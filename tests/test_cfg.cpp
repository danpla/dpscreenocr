
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "dpso/dpso.h"
#include "dpso_utils/cfg.h"
#include "dpso_utils/cfg_ext.h"

#include "flow.h"
#include "utils.h"


static void testGet(
    const DpsoCfg* cfg,
    const char* key,
    const char* defaultVal,
    const char* expectedVal,
    int lineNum)
{
    const auto* gotVal = dpsoCfgGetStr(cfg, key, defaultVal);
    if (std::strcmp(gotVal, expectedVal) == 0)
        return;

    std::fprintf(
        stderr,
        "line %i: dpsoGetStr(\"%s\", \"%s\"): "
        "expected \"%s\", got \"%s\"\n",
        lineNum,
        key,
        defaultVal,
        test::utils::escapeStr(expectedVal).c_str(),
        test::utils::escapeStr(gotVal).c_str());
    test::failure();
}


static void testGet(
    const DpsoCfg* cfg,
    const char* key,
    int defaultVal,
    int expectedVal,
    int lineNum)
{
    const auto gotVal = dpsoCfgGetInt(cfg, key, defaultVal);
    if (gotVal == expectedVal)
        return;

    std::fprintf(
        stderr,
        "line %i: dpsoGetInt(\"%s\", %i): "
        "expected %i, got %i\n",
        lineNum,
        key,
        defaultVal,
        expectedVal,
        gotVal);
    test::failure();
}


static void testGet(
    const DpsoCfg* cfg,
    const char* key,
    bool defaultVal,
    bool expectedVal,
    int lineNum)
{
    const bool gotVal = dpsoCfgGetBool(cfg, key, defaultVal);
    if (gotVal == expectedVal)
        return;

    std::fprintf(
        stderr,
        "line %i: dpsoGetBool(\"%s\", %s) "
        "expected %s, got %s\n",
        lineNum,
        key,
        test::utils::boolToStr(defaultVal).c_str(),
        test::utils::boolToStr(expectedVal).c_str(),
        test::utils::boolToStr(gotVal).c_str());
    test::failure();
}


static std::string hotkeyToStr(const DpsoHotkey& hotkey)
{
    return dpsoHotkeyToString(&hotkey);
}


static void testGet(
    const DpsoCfg* cfg,
    const char* key,
    const DpsoHotkey& defaultVal,
    const DpsoHotkey& expectedVal,
    int lineNum)
{
    DpsoHotkey gotVal;
    dpsoCfgGetHotkey(cfg, key, &gotVal, &defaultVal);

    if (gotVal == expectedVal)
        return;

    std::fprintf(
        stderr,
        "line %i: dpsoCfgGetHotkey(\"%s\", &, {%s}) "
        "expected {%s}, got {%s}\n",
        lineNum,
        key,
        hotkeyToStr(defaultVal).c_str(),
        hotkeyToStr(expectedVal).c_str(),
        hotkeyToStr(gotVal).c_str());
    test::failure();
}


#define TEST_GET(cfg, key, defaultVal, expectedVal) \
    testGet(cfg, key, defaultVal, expectedVal, __LINE__)


static void getDefault(const DpsoCfg* cfg)
{
    TEST_GET(cfg, "default", "", "");
    TEST_GET(cfg, "default", "default_value", "default_value");
    TEST_GET(cfg, "default", 0, 0);
    TEST_GET(cfg, "default", 9, 9);
    TEST_GET(cfg, "default", false, false);
    TEST_GET(cfg, "default", true, true);
}


static void testWhitespaceStr(DpsoCfg* cfg, bool set)
{
    static const char* const whitespaceStrings[] = {
        " ",
        " a",
        "a ",
        " \001 \b\f\n\r\t ",
    };

    static const auto numWhitespaceStrings = (
        sizeof(whitespaceStrings) / sizeof(*whitespaceStrings));

    for (std::size_t i = 0; i < numWhitespaceStrings; ++i) {
        static char key[32];
        std::snprintf(
            key, sizeof(key),
            "str_whitespace_%i", static_cast<int>(i));

        const auto* str = whitespaceStrings[i];
        if (set)
            dpsoCfgSetStr(cfg, key, str);
        else
            TEST_GET(cfg, key, "", str);
    }
}


const char* const backslashTestString = R"(\b\f\n\r\t\\ \z)";


static void setString(DpsoCfg* cfg)
{
    testWhitespaceStr(cfg, true);

    dpsoCfgSetStr(cfg, "str_empty", "");
    dpsoCfgSetStr(cfg, "str", "foo");

    dpsoCfgSetStr(cfg, "str_int_123", "123");
    dpsoCfgSetStr(cfg, "str_int_0", "0");
    dpsoCfgSetStr(cfg, "str_float", "123.5");

    dpsoCfgSetStr(cfg, "str_bool_1", "fAlSe");
    dpsoCfgSetStr(cfg, "str_bool_2", "TrUe");

    dpsoCfgSetStr(cfg, "str_backslash_test", backslashTestString);
}


static void getString(DpsoCfg* cfg)
{
    testWhitespaceStr(cfg, false);

    TEST_GET(cfg, "str_default", "default", "default");

    TEST_GET(cfg, "str_empty", "default", "");

    TEST_GET(cfg, "str", "", "foo");
    TEST_GET(cfg, "str", 0, 0);
    TEST_GET(cfg, "str", true, true);

    TEST_GET(cfg, "str_int_123", "", "123");
    TEST_GET(cfg, "str_int_123", 0, 123);
    TEST_GET(cfg, "str_int_123", false, true);

    TEST_GET(cfg, "str_int_0", "", "0");
    TEST_GET(cfg, "str_int_0", 1, 0);
    TEST_GET(cfg, "str_int_0", true, false);

    // There are currently no routines to get/set floats, so don't
    // treat them as integers, like strto*() do.
    TEST_GET(cfg, "str_float", 0, 0);

    TEST_GET(cfg, "str_bool_1", true, false);
    TEST_GET(cfg, "str_bool_2", false, true);

    TEST_GET(cfg, "str_backslash_test", "", backslashTestString);
}


static void setInt(DpsoCfg* cfg)
{
    dpsoCfgSetInt(cfg, "int_123", 123);
    dpsoCfgSetInt(cfg, "int_0", 0);
}


static void getInt(const DpsoCfg* cfg)
{
    TEST_GET(cfg, "int_123", "", "123");
    TEST_GET(cfg, "int_123", 0, 123);
    TEST_GET(cfg, "int_123", false, true);

    TEST_GET(cfg, "int_0", "", "0");
    TEST_GET(cfg, "int_0", 1, 0);
    TEST_GET(cfg, "int_0", true, false);
}


static void setBool(DpsoCfg* cfg)
{
    dpsoCfgSetBool(cfg, "bool_true", true);
    dpsoCfgSetBool(cfg, "bool_false", false);
}


static void getBool(const DpsoCfg* cfg)
{
    TEST_GET(cfg, "bool_true", "", "true");
    TEST_GET(cfg, "bool_true", 0, 1);
    TEST_GET(cfg, "bool_true", false, true);

    TEST_GET(cfg, "bool_false", "", "false");
    TEST_GET(cfg, "bool_false", 1, 0);
    TEST_GET(cfg, "bool_false", true, false);
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
    const DpsoHotkey defaultHotkey {dpsoKeyF1, dpsoKeyModWin};

    TEST_GET(cfg, "hotkey_default", defaultHotkey, defaultHotkey);

    for (const auto& test : hotkeyTests)
        TEST_GET(cfg, test.key, defaultHotkey, test.hotkey);
}


const char* const cfgFileName = "test_cfg_file.cfg";


static void reload(DpsoCfg* cfg)
{
    if (!dpsoCfgSave(cfg, cfgFileName)) {
        std::fprintf(
            stderr,
            "reload(): dpsoCfgSave(cfg, \"%s\") failed: %s\n",
            cfgFileName,
            dpsoGetError());
        std::exit(EXIT_FAILURE);
    }

    const auto loaded = dpsoCfgLoad(cfg, cfgFileName);
    std::remove(cfgFileName);

    if (!loaded) {
        std::fprintf(
            stderr,
            "reload(): dpsoCfgLoad(cfg, \"%s\") failed: %s\n",
            cfgFileName,
            dpsoGetError());
        std::exit(EXIT_FAILURE);
    }
}


static void loadCfgData(DpsoCfg* cfg, const char* cfgData)
{
    auto* fp = std::fopen(cfgFileName, "wb");
    if (!fp) {
        std::fprintf(
            stderr,
            "loadCfgData(): fopen(\"%s\", \"wb\") failed: %s\n",
            cfgFileName,
            std::strerror(errno));
        std::exit(EXIT_FAILURE);
    }

    std::fputs(cfgData, fp);
    std::fclose(fp);

    const auto loaded = dpsoCfgLoad(cfg, cfgFileName);
    std::remove(cfgFileName);

    if (!loaded) {
        std::fprintf(
            stderr,
            "loadCfgData(): dpsoCfgLoad(cfg, \"%s\") failed: %s\n",
            cfgFileName,
            dpsoGetError());
        std::exit(EXIT_FAILURE);
    }
}


static void testValueOverridingOnLoad(DpsoCfg* cfg)
{
    loadCfgData(cfg, "key value1\nkey value2\n");
    TEST_GET(cfg, "key", "", "value2");
}


static void testUnescapedBackslashAtEndOfQuotedString(DpsoCfg* cfg)
{
    loadCfgData(cfg, "key \"a\\\"");
    TEST_GET(cfg, "key", "", "a");
}


static void testCfg()
{
    dpso::CfgUPtr cfg{dpsoCfgCreate()};
    if (!cfg) {
        std::fprintf(
            stderr,
            "dpsoCfgCreate() failed: %s\n",
            dpsoGetError());
        std::exit(EXIT_FAILURE);
    }

    setString(cfg.get());
    setInt(cfg.get());
    setBool(cfg.get());
    setHotkey(cfg.get());

    reload(cfg.get());

    getDefault(cfg.get());

    getString(cfg.get());
    getInt(cfg.get());
    getBool(cfg.get());
    getHotkey(cfg.get());

    testValueOverridingOnLoad(cfg.get());
    testUnescapedBackslashAtEndOfQuotedString(cfg.get());
}


REGISTER_TEST(testCfg);
