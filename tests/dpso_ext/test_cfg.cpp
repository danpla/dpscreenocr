
#include <climits>
#include <cstring>
#include <initializer_list>
#include <string>

#include "dpso/dpso.h"
#include "dpso_ext/cfg.h"
#include "dpso_ext/cfg_ext.h"
#include "dpso_utils/error_get.h"
#include "dpso_utils/str.h"

#include "flow.h"
#include "utils.h"


using namespace dpso;


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


void testGetStr(
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
        "dpsoGetStr(\"{}\", {}): expected {}, got {}\n",
        key,
        test::utils::toStr(defaultVal),
        test::utils::toStr(expectedVal),
        test::utils::toStr(gotVal));
}


void testGetInt(
    const DpsoCfg* cfg,
    const char* key,
    int expectedVal,
    int defaultVal)
{
    const auto gotVal = dpsoCfgGetInt(cfg, key, defaultVal);
    if (gotVal == expectedVal)
        return;

    test::failure(
        "dpsoGetInt(\"{}\", {}): expected {}, got {}\n",
        key,
        defaultVal,
        expectedVal,
        gotVal);
}


void testGetBool(
    const DpsoCfg* cfg,
    const char* key,
    bool expectedVal,
    bool defaultVal)
{
    const auto gotVal = dpsoCfgGetBool(cfg, key, defaultVal);
    if (gotVal == expectedVal)
        return;

    test::failure(
        "dpsoGetBool(\"{}\", {}): expected {}, got {}\n",
        key,
        test::utils::toStr(defaultVal),
        test::utils::toStr(expectedVal),
        test::utils::toStr(gotVal));
}


const auto* const nonexistentKey = "nonexistent_key";


const std::initializer_list<BasicTypesTest> strTests{
    {"str_empty", "", "a", 5, 5, false, false},
    {"str_leading_space", " a", "", 5, 5, false, false},
    {"str_trailing_space", "a ", "", 5, 5, false, false},
    {"str_single_space", " ", "", 5, 5, false, false},
    {"str_control_chars", "\b\f\n\r\t", "", 5, 5, false, false},
    {"str_escapes", R"(\b\f\n\r\t\\ \z)", "", 5, 5, false, false},
    {"str_normal", "foo", "", 5, 5, false, false},
    {"str_int_0", "0", "", 0, 1, true, true},
    {"str_int_5", "5", "", 5, 0, false, false},
    {"str_int_minus_5", "-5", "", -5, 0, false, false},
    {"str_int_leading_space", " 5", "", 0, 0, false, false},
    {"str_int_trailing_space", "5 ", "", 0, 0, false, false},
    {"str_int_leading_char", "a5", "", 0, 0, false, false},
    {"str_int_trailing_char", "5a", "", 0, 0, false, false},
    {"str_float_period", "5.5", "", 0, 0, true, true},
    {"str_float_comma", "5,5", "", 0, 0, true, true},
    {"str_bool_true", "TrUe", "", 5, 5, true, false},
    {"str_bool_false", "FaLsE", "", 5, 5, false, true},
};


const std::initializer_list<BasicTypesTest> intTests{
    {"int_0", "0", "", 0, 5, true, true},
    {"int_5", "5", "", 5, 0, false, false},
    {"int_minus_5", "-5", "", -5, 0, false, false},
};


const std::initializer_list<BasicTypesTest> boolTests{
    {"bool_true", "true", "", 5, 5, true, false},
    {"bool_false", "false", "", 5, 5, false, true},
};


std::string makeCfgKeyForChar(char c)
{
    return "str_char_" + str::rightJustify(str::toStr(c, 16), 2, '0');
}


void setStrByteChars(DpsoCfg* cfg)
{
    for (int c = 1; c < 256; ++c) {
        const char val[2] = {static_cast<char>(c), 0};
        dpsoCfgSetStr(cfg, makeCfgKeyForChar(c).c_str(), val);
    }
}


void getStrByteChars(const DpsoCfg* cfg)
{
    for (int c = 1; c < 256; ++c) {
        const char val[2] = {static_cast<char>(c), 0};
        testGetStr(cfg, makeCfgKeyForChar(c).c_str(), val, "");
    }
}


void setBasicTypes(DpsoCfg* cfg)
{
    for (const auto& test : strTests)
        dpsoCfgSetStr(cfg, test.key, test.strVal);

    setStrByteChars(cfg);

    for (const auto& test : intTests)
        dpsoCfgSetInt(cfg, test.key, test.intVal);

    for (const auto& test : boolTests)
        dpsoCfgSetBool(cfg, test.key, test.boolVal);
}


void getBasicTypes(const DpsoCfg* cfg)
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


std::string toStr(const DpsoHotkey& hotkey)
{
    return dpsoHotkeyToString(&hotkey);
}


void testGetHotkey(
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
        "dpsoCfgGetHotkey(\"{}\", &, [{}]): "
        "expected [{}], got [{}]\n",
        key,
        toStr(defaultVal),
        toStr(expectedVal),
        toStr(gotVal));
}


namespace {


struct HotkeyTest {
    const char* key;
    DpsoHotkey hotkey;
};


}


const HotkeyTest hotkeyTests[] = {
    {"hotkey_none", {dpsoNoKey, dpsoNoKeyMods}},
    {
        "hotkey_mods_only",
        {dpsoNoKey, dpsoKeyModShift | dpsoKeyModCtrl}},
    {"hotkey_key_only", {dpsoKeyA, dpsoNoKeyMods}},
    {"hotkey", {dpsoKeyA, dpsoKeyModShift | dpsoKeyModCtrl}},
};


void setHotkey(DpsoCfg* cfg)
{
    for (const auto& test : hotkeyTests)
        dpsoCfgSetHotkey(cfg, test.key, &test.hotkey);
}


void getHotkey(const DpsoCfg* cfg)
{
    const DpsoHotkey defaultHotkey{dpsoKeyF1, dpsoKeyModWin};

    testGetHotkey(cfg, nonexistentKey, defaultHotkey, defaultHotkey);
    for (const auto& test : hotkeyTests)
        testGetHotkey(cfg, test.key, test.hotkey, defaultHotkey);
}


const auto* const cfgFileName = "test_cfg_file.cfg";


void reload(DpsoCfg* cfg)
{
    if (!dpsoCfgSave(cfg, cfgFileName))
        test::fatalError(
            "reload(): dpsoCfgSave(cfg, \"{}\"): {}\n",
            cfgFileName,
            dpsoGetError());

    const auto loaded = dpsoCfgLoad(cfg, cfgFileName);
    test::utils::removeFile(cfgFileName);

    if (!loaded)
        test::fatalError(
            "reload(): dpsoCfgLoad(cfg, \"{}\"): {}\n",
            cfgFileName,
            dpsoGetError());
}


void loadCfgData(DpsoCfg* cfg, const char* cfgData)
{
    test::utils::saveText("loadCfgData", cfgFileName, cfgData);

    const auto loaded = dpsoCfgLoad(cfg, cfgFileName);
    test::utils::removeFile(cfgFileName);

    if (!loaded)
        test::fatalError(
            "loadCfgData(): dpsoCfgLoad(cfg, \"{}\"): {}\n",
            cfgFileName,
            dpsoGetError());
}


void testValueOverridingOnLoad(DpsoCfg* cfg)
{
    const std::string key = "duplicate_key";

    loadCfgData(cfg, (key + " value1\n" + key + " value2").c_str());
    testGetStr(cfg, key.c_str(), "value2", "");
}


void testStrParsing(DpsoCfg* cfg)
{
    const struct Test {
        const char* key;
        const char* valInFile;
        const char* expectedVal;
    } tests[] = {
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


void testIntParsing(DpsoCfg* cfg)
{
    const struct Test {
        const char* key;
        std::string valInFile;
        int expectedVal;
        int defaultVal;
    } tests[] = {
        {"int_minus_0", "-0", 0, 1},
        {"int_123", "123", 123, 0},
        {"int_minus_123", "-123", -123, 0},
        {"int_min", str::toStr(INT_MIN), INT_MIN, 0},
        {"int_max", str::toStr(INT_MAX), INT_MAX, 0},
        {"int_too_big", "9999999999999999999", 1, 1},
        {"int_too_small", "-9999999999999999999", 1, 1},
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


void testBoolParsing(DpsoCfg* cfg)
{
    const struct Test {
        const char* key;
        const char* valInFile;
        bool expectedVal;
        bool defaultVal;
    } tests[] = {
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
        testGetBool(cfg, test.key, test.expectedVal, test.defaultVal);
    }
}


void testHotkeyParsing(DpsoCfg* cfg)
{
    const struct Test {
        const char* key;
        const char* valInFile;
        DpsoHotkey expectedVal;
        DpsoHotkey defaultVal;
    } tests[] = {
        {
            "hotkey_empty",
            "",
            dpsoEmptyHotkey,
            {dpsoKeyA, dpsoNoKeyMods}},
        {
            "hotkey_a_upper",
            "A",
            {dpsoKeyA, dpsoNoKeyMods},
            dpsoEmptyHotkey},
        {
            "hotkey_a_lower",
            "a",
            {dpsoKeyA, dpsoNoKeyMods},
            dpsoEmptyHotkey},
        {
            "hotkey_ctrl_a",
            "ctRL+A",
            {dpsoKeyA, dpsoKeyModCtrl},
            dpsoEmptyHotkey},
        {
            "hotkey_ctrl_a_a",
            "Ctrl+A+A",
            dpsoEmptyHotkey,
            {dpsoKeyA, dpsoKeyModCtrl}},
        {
            "hotkey_ctrl_ctrl_a",
            "Ctrl+Ctrl+A",
            dpsoEmptyHotkey,
            {dpsoKeyA, dpsoKeyModCtrl}},
        {
            "hotkey_a_ctrl",
            "A+Ctrl",
            dpsoEmptyHotkey,
            {dpsoKeyA, dpsoKeyModCtrl}},
        {
            "hotkey_with_blanks",
            " \t Ctrl \t + \t A \t ",
            {dpsoKeyA, dpsoKeyModCtrl},
            dpsoEmptyHotkey},
        {
            "hotkey_with_cr",
            "\rA",
            dpsoEmptyHotkey,
            {dpsoKeyA, dpsoNoKeyMods}},
        {
            "hotkey_with_lf",
            "\nA",
            dpsoEmptyHotkey,
            {dpsoKeyA, dpsoNoKeyMods}},
        {
            "hotkey_leading_plus",
            "+A",
            dpsoEmptyHotkey,
            {dpsoKeyA, dpsoNoKeyMods}},
        {
            "hotkey_trailing_plus",
            "A+",
            dpsoEmptyHotkey,
            {dpsoKeyA, dpsoNoKeyMods}},
        {
            "hotkey_extra_plus_inside",
            "Ctrl++A",
            dpsoEmptyHotkey,
            {dpsoKeyA, dpsoKeyModCtrl}},
        {
            "hotkey_keypad_plus",
            "Keypad +",
            {dpsoKeyKpPlus, dpsoNoKeyMods},
            dpsoEmptyHotkey},
        {
            "hotkey_ctrl",
            "Ctrl",
            {dpsoNoKey, dpsoKeyModCtrl},
            dpsoEmptyHotkey},
        {
            "hotkey_option",
            "Option",
            {dpsoNoKey, dpsoKeyModAlt},
            dpsoEmptyHotkey},
        {
            "hotkey_command",
            "Command",
            {dpsoNoKey, dpsoKeyModWin},
            dpsoEmptyHotkey},
        {
            "hotkey_super",
            "Super",
            {dpsoNoKey, dpsoKeyModWin},
            dpsoEmptyHotkey},
    };

    for (const auto& test : tests) {
        loadCfgData(
            cfg,
            (std::string{test.key} + " " + test.valInFile).c_str());
        testGetHotkey(
            cfg, test.key, test.expectedVal, test.defaultVal);
    }
}


void testKeyValidity(DpsoCfg* cfg)
{
    const struct Test {
        const char* key;
        bool isValid;
    } tests[] = {
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
        {"\xff", true},
    };

    for (const auto& test : tests) {
        dpsoCfgSetStr(cfg, test.key, "");
        const auto wasSet = dpsoCfgKeyExists(cfg, test.key);
        if (wasSet == test.isValid)
            continue;

        test::failure(
            "testKeyValidity: Key \"{}\" is expected to be {} and "
            "{} be set\n",
            test::utils::escapeStr(test.key),
            test.isValid ? "valid" : "invalid",
            test.isValid ? "should" : "should not");
    }
}

void testKeyCaseInsensitivity(DpsoCfg* cfg)
{
    dpsoCfgSetStr(cfg, "Case_Insensitivity_TEST", "");
    if (!dpsoCfgKeyExists(cfg, "case_insensitivity_test"))
        test::failure("testKeyCaseInsensitivity failed\n");
}


void testSavedValueFormat()
{
    static const auto* key = "key";

    const struct Test {
        const char* val;
        std::string expectedData;
        Test(const char* val, const char* expectedValData)
            : val{val}
            , expectedData{
                test::utils::lfToNativeNewline(
                    str::format("key {}\n", expectedValData).c_str())}
        {
        }
    } tests[] = {
        {"", ""},
        {" ", "\\ \\"},
        {" a", "\\ a"},
        {"a ", "a \\"},
        {" a ", "\\ a \\"},
        {"\n\r\t\\", "\\n\\r\\t\\\\"},
        {"\b\f\v\x01\xff", "\b\f\v\x01\xff"},
    };

    dpso::CfgUPtr cfg{dpsoCfgCreate()};
    if (!cfg)
        test::fatalError(
            "testSavedValueFormat(): dpsoCfgCreate(): {}\n",
            dpsoGetError());

    for (const auto& test : tests) {
        dpsoCfgSetStr(cfg.get(), key, test.val);
        if (!dpsoCfgSave(cfg.get(), cfgFileName))
            test::fatalError(
                "testSavedValueFormat(): dpsoCfgSave(cfg, \"{}\"): "
                "{}\n",
                cfgFileName,
                dpsoGetError());

        const auto gotData = test::utils::loadText(
            "testSavedValueFormat", cfgFileName);
        if (gotData == test.expectedData)
            continue;

        test::failure(
            "testSavedValueFormat(): Unexpected value format\n");
        test::utils::printFirstDifference(
            test.expectedData.c_str(), gotData.c_str());
    }

    test::utils::removeFile(cfgFileName);
}


void testCfg()
{
    dpso::CfgUPtr cfg{dpsoCfgCreate()};
    if (!cfg)
        test::fatalError("dpsoCfgCreate(): {}\n", dpsoGetError());

    setBasicTypes(cfg.get());
    setHotkey(cfg.get());

    reload(cfg.get());

    getBasicTypes(cfg.get());
    getHotkey(cfg.get());

    testValueOverridingOnLoad(cfg.get());
    testStrParsing(cfg.get());
    testIntParsing(cfg.get());
    testBoolParsing(cfg.get());
    testHotkeyParsing(cfg.get());

    testKeyValidity(cfg.get());
    testKeyCaseInsensitivity(cfg.get());

    testSavedValueFormat();
}


}


REGISTER_TEST(testCfg);
