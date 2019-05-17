
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "dpso/dpso.h"
#include "dpso_utils/cfg_ext.h"
#include "dpso_utils/cfg_private.h"

#include "flow.h"
#include "utils.h"


static void testGet(
    const char* key,
    const char* defaultVal,
    const char* expectedVal,
    int lineNum)
{
    const auto* gotVal = dpsoCfgGetStr(key, defaultVal);
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
    const char* key,
    int defaultVal,
    int expectedVal,
    int lineNum)
{
    const auto gotVal = dpsoCfgGetInt(key, defaultVal);
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


static const char* boolToStr(bool b)
{
    return b ? "true" : "false";
}


static void testGet(
    const char* key,
    bool defaultVal,
    bool expectedVal,
    int lineNum)
{
    const bool gotVal = dpsoCfgGetBool(key, defaultVal);
    if (gotVal == expectedVal)
        return;

    std::fprintf(
        stderr,
        "line %i: dpsoGetBool(\"%s\", %s) "
        "expected %s, got %s\n",
        lineNum,
        key,
        boolToStr(defaultVal),
        boolToStr(expectedVal),
        boolToStr(gotVal));
    test::failure();
}


static void testGet(
    const char* key,
    const DpsoHotkey& defaultVal,
    const DpsoHotkey& expectedVal,
    int lineNum)
{
    DpsoHotkey gotVal;
    dpsoCfgGetHotkey(key, &gotVal, &defaultVal);

    if (gotVal.key == expectedVal.key
            && gotVal.mods == expectedVal.mods)
        return;

    std::fprintf(
        stderr,
        "line %i: dpsoCfgGetHotkey(\"%s\", &, {%s}) "
        "expected {%s}, got {%s}\n",
        lineNum,
        key,
        dpsoHotkeyToString(&defaultVal),
        dpsoHotkeyToString(&expectedVal),
        dpsoHotkeyToString(&gotVal));
    test::failure();
}


#define TEST_GET(key, defaultVal, expectedVal) \
    testGet(key, defaultVal, expectedVal, __LINE__)


static void getDefault()
{
    TEST_GET("default", "", "");
    TEST_GET("default", "default_value", "default_value");
    TEST_GET("default", 0, 0);
    TEST_GET("default", 9, 9);
    TEST_GET("default", false, false);
    TEST_GET("default", true, true);
}


static void testWhitespaceStr(bool set)
{
    static const char* whitespaceStrings[] = {
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
            "str_whitespace_%i", static_cast<int>(i + 1));

        const auto* str = whitespaceStrings[i];
        if (set)
            dpsoCfgSetStr(key, str);
        else
            TEST_GET(key, "", str);
    }
}


static const char* testBackslash = "\\b\\f\\n\\r\\t \\z";


static void setString()
{
    testWhitespaceStr(true);

    dpsoCfgSetStr("str_empty", "");
    dpsoCfgSetStr("str", "foo");

    dpsoCfgSetStr("str_int_123", "123");
    dpsoCfgSetStr("str_int_0", "0");
    dpsoCfgSetStr("str_float", "123.5");

    dpsoCfgSetStr("str_bool_1", "fAlSe");
    dpsoCfgSetStr("str_bool_2", "TrUe");

    dpsoCfgSetStr("str_backslast_test", testBackslash);
}


static void getString()
{
    testWhitespaceStr(false);

    TEST_GET("str_default", "default", "default");

    TEST_GET("str_empty", "default", "");

    TEST_GET("str", "", "foo");
    TEST_GET("str", 0, 0);
    TEST_GET("str", true, true);

    TEST_GET("str_int_123", "", "123");
    TEST_GET("str_int_123", 0, 123);
    TEST_GET("str_int_123", false, true);

    TEST_GET("str_int_0", "", "0");
    TEST_GET("str_int_0", 1, 0);
    TEST_GET("str_int_0", true, false);

    // There are currently no routines to get/set floats, so don't
    // tread them as integers, like strto*() routines do.
    TEST_GET("str_float", 0, 0);

    TEST_GET("str_bool_1", true, false);
    TEST_GET("str_bool_2", false, true);

    TEST_GET("str_backslast_test", "", testBackslash);
}


static void setInt()
{
    dpsoCfgSetInt("int_123", 123);
    dpsoCfgSetInt("int_0", 0);
}


static void getInt()
{
    TEST_GET("int_123", "", "123");
    TEST_GET("int_123", 0, 123);
    TEST_GET("int_123", false, true);

    TEST_GET("int_0", "", "0");
    TEST_GET("int_0", 1, 0);
    TEST_GET("int_0", true, false);
}


static void setBool()
{
    dpsoCfgSetBool("bool_true", true);
    dpsoCfgSetBool("bool_false", false);
}


static void getBool()
{
    TEST_GET("bool_true", "", "true");
    TEST_GET("bool_true", 0, 1);
    TEST_GET("bool_true", false, true);

    TEST_GET("bool_false", "", "false");
    TEST_GET("bool_false", 1, 0);
    TEST_GET("bool_false", true, false);
}


namespace {


struct HotkeyTest {
    const char* key;
    DpsoHotkey set;
    DpsoHotkey get;
};


}


const HotkeyTest hotkeyTests[] = {
    {
        "hotkey_none",
        {dpsoUnknownKey, dpsoKeyModNone},
        {dpsoUnknownKey, dpsoKeyModNone}
    },
    {
        "hotkey_mods_only",
        {dpsoUnknownKey, dpsoKeyModShift | dpsoKeyModCtrl},
        {dpsoUnknownKey, dpsoKeyModNone}
    },
    {
        "hotkey_key_only",
        {dpsoKeyA, dpsoKeyModNone},
        {dpsoKeyA, dpsoKeyModNone}
    },
    {
        "hotkey",
        {dpsoKeyA, dpsoKeyModShift | dpsoKeyModCtrl},
        {dpsoKeyA, dpsoKeyModShift | dpsoKeyModCtrl}
    },
};


static void setHotkey()
{
    for (const auto& test : hotkeyTests)
        dpsoCfgSetHotkey(test.key, &test.set);
}


static void getHotkey()
{
    const DpsoHotkey defaultHotkey {dpsoKeyF1, dpsoKeyModWin};

    TEST_GET("hotkey_default", defaultHotkey, defaultHotkey);

    for (const auto& test : hotkeyTests)
        TEST_GET(test.key, defaultHotkey, test.get);
}


static void reload()
{
    auto* fp = std::tmpfile();
    if (!fp)
        return;

    dpsoCfgSaveFp(fp);
    dpsoCfgClear();

    std::rewind(fp);
    dpsoCfgLoadFp(fp);

    std::fclose(fp);
}


static void testCfg()
{
    dpsoCfgClear();

    setString();
    setInt();
    setBool();
    setHotkey();

    reload();

    getDefault();

    getString();
    getInt();
    getBool();
    getHotkey();
}


REGISTER_TEST("cfg", testCfg);
