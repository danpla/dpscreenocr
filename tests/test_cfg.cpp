
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>

#include "dpso/dpso.h"
#include "dpso/str.h"
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

    bool boolVal;
    bool boolValDefault;
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
    bool defaultVal)
{
    const auto gotVal = dpsoCfgGetBool(cfg, key, defaultVal);
    if (gotVal == expectedVal)
        return;

    test::failure(
        "dpsoGetBool(\"%s\", %s): expected %s, got %s\n",
        key,
        test::utils::boolToStr(defaultVal).c_str(),
        test::utils::boolToStr(expectedVal).c_str(),
        test::utils::boolToStr(gotVal).c_str());
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
};


const std::string makeCfgKeyForChar(char c)
{
    return dpso::str::printf("str_char_%02hhx", c);
}


static void setStrByteChars(DpsoCfg* cfg)
{
    for (int c = 1; c < 256; ++c) {
        const char val[2] = {static_cast<char>(c), 0};
        dpsoCfgSetStr(cfg, makeCfgKeyForChar(c).c_str(), val);
    }
}


static void getStrByteChars(const DpsoCfg* cfg)
{
    for (int c = 1; c < 256; ++c) {
        const char val[2] = {static_cast<char>(c), 0};
        testGetStr(
            cfg, makeCfgKeyForChar(c).c_str(), val, "");
    }
}


static void setBasicTypes(DpsoCfg* cfg)
{
    for (const auto& test : strTests)
        dpsoCfgSetStr(cfg, test.key, test.strVal);

    setStrByteChars(cfg);

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

    getStrByteChars(cfg);

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


static void testStrValueParsing(DpsoCfg* cfg)
{
    struct Test {
        const char* key;
        const char* valInFile;
        const char* expectedVal;
    };

    const Test tests[] = {
        {"space", " ", ""},
        {"line_feed", "\n", ""},
        {"carriage_return", "\r", ""},
        {"tab", "\t", ""},
        {"vertical_tab", "\v", "\v"},
        {"backspace", "\b", "\b"},
        {"form_feed", "\f", "\f"},
        {"byte_x01", "\x01", "\x01"},
        {"byte_xff", "\xff", "\xff"},
        {"backslash_n", "\\n", "\n"},
        {"backslash_r", "\\r", "\r"},
        {"backslash_t", "\\t", "\t"},
        {"backslash_v", "\\v", "v"},
        {"backslash_b", "\\b", "b"},
        {"backslash_f", "\\f", "f"},
        {"backslash_a", "\\a", "a"},
        {"double_quote", "\"", "\""},
        {"two_double_quotes", "\"\"", "\"\""},
        {"space_a", " a", "a"},
        {"a_space", "a ", "a"},
        {"backslash_space", "\\ ", " "},
        {"backslash_space_space", "\\  ", " "},
        {"backslash", "\\", ""},
        {"backslash_newline", "\\\nkey value", ""},
        {"backslash_backslash", "\\\\", "\\"},
        {"backslash_space_backslash", "\\ \\", " "},
        {"backslash_space_backslash_space", "\\ \\ ", "  "},
        {"backslash_space_a", "\\ a", " a"},
        {"a_space_backslash", "a \\", "a "},
        {"backslash_space_a_space_backslash", "\\ a \\", " a "},
        {"backslash_space_tab_space_backslash", "\\ \t \\", " \t "},

    };

    for (const auto& test : tests) {
        loadCfgData(
            cfg,
            (std::string{test.key} + " " + test.valInFile).c_str());
        testGetStr(cfg, test.key, test.expectedVal, "default value");
    }
}


static void testIntValueParsing(DpsoCfg* cfg)
{
    struct Test {
        const char* key;
        const char* valInFile;
        int expectedVal;
        int defaultVal;
    };

    const Test tests[] = {
        {"int_minus_0", "-0", 0, 1},
        {"int_123", "123", 123, 0},
        {"int_minus_123", "-123", -123, 0},
        {"int_too_big", "999999999999999999999999", INT_MAX, 0},
        {"int_too_small", "-999999999999999999999999", INT_MIN, 0},
        {"int_leading_0x", "0x01", 5, 5},
        {"int_leading_0", "012", 12, 1},
    };

    for (const auto& test : tests) {
        loadCfgData(
            cfg,
            (std::string{test.key} + " " + test.valInFile).c_str());
        testGetInt(cfg, test.key, test.expectedVal, test.defaultVal);
    }
}


static void testBoolValueParsing(DpsoCfg* cfg)
{
    struct Test {
        const char* key;
        const char* valInFile;
        bool expectedVal;
        bool defaultVal;
    };

    const Test tests[] = {
        {"bool_True", "True", true, false},
        {"bool_TRUE", "TRUE", true, false},
        {"bool_TRUE_x", "TRUE x", false, false},
        {"bool_False", "False", false, true},
        {"bool_FALSE", "FALSE", false, true},
    };

    for (const auto& test : tests) {
        loadCfgData(
            cfg,
            (std::string{test.key} + " " + test.valInFile).c_str());
        testGetInt(cfg, test.key, test.expectedVal, test.defaultVal);
    }
}


static void testKeyValidity(DpsoCfg* cfg)
{
    struct Test {
        const char* key;
        bool isValid;
    };

    const Test tests[] = {
        {"", false},
        {" ", false},
        {" a", false},
        {"a ", false},
        {"a a", false},
        {"\t", false},
        {"\r", false},
        {"\n", false},
        {"\f", true},
        {"\v", true},
        {"\x01", true},
    };

    for (const auto& test : tests) {
        dpsoCfgSetStr(cfg, test.key, "");
        const auto wasSet = dpsoCfgKeyExists(cfg, test.key);
        if (wasSet == test.isValid)
            continue;

        test::failure(
            "testKeyValidity: \"%s\" key is expected to be %s and "
            "%s be set\n",
            test::utils::escapeStr(test.key).c_str(),
            test.isValid ? "valid" : "invalid",
            test.isValid ? "should" : "should not");
    }
}

static void testKeyCaseInsensitivity(DpsoCfg* cfg)
{
    dpsoCfgSetStr(cfg, "Case_Insensitivity_TEST", "");
    if (!dpsoCfgKeyExists(cfg, "case_insensitivity_test"))
        test::failure("testKeyCaseInsensitivity failed\n");
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
    testStrValueParsing(cfg.get());
    testIntValueParsing(cfg.get());
    testBoolValueParsing(cfg.get());

    testKeyValidity(cfg.get());
    testKeyCaseInsensitivity(cfg.get());
}


REGISTER_TEST(testCfg);
