
#include "test_windows_utils.h"

#include <cstdio>

#include "dpso_utils/windows_utils.h"

#include "flow.h"
#include "utils.h"


static void testUtfConverion(const char* utf8Str)
{
    using namespace dpso::win;

    std::wstring utf16Str;

    try {
        utf16Str = utf8ToUtf16(utf8Str);
    } catch (std::runtime_error& e) {
        std::fprintf(
            stderr,
            "utf8ToUtf16(\"%s\") failed: %s\n",
            test::utils::escapeStr(utf8Str).c_str(), e.what());
        test::failure();
    }

    std::string utf8ToUtf16Result;
    try {
        utf8ToUtf16Result = utf16ToUtf8(utf16Str);
    } catch (std::runtime_error& e) {
        std::fprintf(
            stderr,
            "utf16ToUtf8() failed to convert result of "
            "utf8ToUtf16(\"%s\"): %s\n",
            test::utils::escapeStr(utf8Str).c_str(), e.what());
        test::failure();
    }

    if (utf8ToUtf16Result != utf8Str) {
        std::fprintf(
            stderr,
            "utf8ToUtf16() <=> utf16ToUtf8() conversion failed.\n"
            "  Original:  %s\n"
            "  Converted: %s\n",
            test::utils::escapeStr(utf8Str).c_str(),
            test::utils::escapeStr(
                utf8ToUtf16Result.c_str()).c_str());
        test::failure();
    }
}


static void testUtfConversions()
{
    const char* utf8Strings[] = {
        // Юникод
        "\320\256\320\275\320\270\320\272\320\276\320\264"
    };

    for (const auto* utf8Str : utf8Strings)
        testUtfConverion(utf8Str);
}


void testWindowsUtils()
{
    testUtfConversions();
}

