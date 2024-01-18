
#include "dpso_utils/windows/utf.h"

#include "flow.h"
#include "utils.h"


namespace {


void testUtfConversion(const char* utf8Str)
{
    using namespace dpso::windows;

    std::wstring utf16Str;

    try {
        utf16Str = utf8ToUtf16(utf8Str);
    } catch (std::runtime_error& e) {
        test::failure(
            "utf8ToUtf16(\"{}\"): {}\n",
            test::utils::escapeStr(utf8Str), e.what());
        return;
    }

    std::string utf16ToUtf8Result;
    try {
        utf16ToUtf8Result = utf16ToUtf8(utf16Str.c_str());
    } catch (std::runtime_error& e) {
        test::failure(
            "utf16ToUtf8() failed to convert result of "
            "utf8ToUtf16(\"{}\"): {}\n",
            test::utils::escapeStr(utf8Str), e.what());
        return;
    }

    if (utf16ToUtf8Result != utf8Str)
        test::failure(
            "utf8ToUtf16() <=> utf16ToUtf8() conversion failed.\n"
            "  Original:  {}\n"
            "  Converted: {}\n",
            test::utils::escapeStr(utf8Str),
            test::utils::escapeStr(utf16ToUtf8Result.c_str()));
}


void testUtfConversions()
{
    const char* const utf8Strings[] = {
        "",
        // Юникод
        "\320\256\320\275\320\270\320\272\320\276\320\264"
    };

    for (const auto* utf8Str : utf8Strings)
        testUtfConversion(utf8Str);
}


void testInvalidUtf8()
{
    using namespace dpso::windows;

    const char* const strings[] = {
        // Invalid start byte 0x80 - 0xbf
        "\x80",
        "\xbf",

        // Surrogates 0xd800 - 0xdfff
        "\xd8\x00",
        "\xdf\xff",

        // Overlong 2 bytes (\xC0\x80 - \xC1\xBF)
        "\xc0\x80",
        "\xc1\xbf",

        // Overlong 3 bytes (\xE0\x80\x80 - \xE0\x9F\xBF)
        "\xe0\x80\x80",
        "\xe0\x9f\xbf",

        // Overlong 4 bytes (\xF0\x80\x80\x80 - \xF0\x8F\xBF\xBF)
        "\xf0\x80\x80\x80",
        "\xf0\x8f\xbf\xbf",

        // Out of bounds (> 0x10ffff)
        "\xf4\x90\x80\x80",
    };

    for (const auto* s : strings) {
        try {
            dpso::windows::utf8ToUtf16(s);
            test::failure(
                "utf8ToUtf16(\"{}\") doesn't throw\n",
                test::utils::escapeStr(s));
        } catch (std::runtime_error& e) {
        }
    }
}


void testInvalidUtf16()
{
    const wchar_t* const strings[] = {
        L"\xd800",  // min leading
        L"\xdbff",  // max leading
        L"\xdc00",  // min trailing
        L"\xdfff",  // max trailing
        L"\xd800\xdbff",  // min leading + (min trailing - 1)
        L"\xd800\xe000",  // min leading + (max trailing + 1)
        L"\xdbff\xdbff",  // max leading + (min trailing - 1)
        L"\xdbff\xe000",  // max leading + (max trailing + 1)
    };

    for (const auto* s : strings) {
        try {
            dpso::windows::utf16ToUtf8(s);
            test::failure(
                "utf16ToUtf8() doesn't throw on invalid UTF-16\n");
        } catch (std::runtime_error& e) {
        }
    }
}


void testWindowsUtf()
{
    testUtfConversions();
    testInvalidUtf8();
    testInvalidUtf16();
}


}


REGISTER_TEST(testWindowsUtf);
