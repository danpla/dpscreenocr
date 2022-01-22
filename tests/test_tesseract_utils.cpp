
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "dpso/ocr_engine/tesseract/utils.h"

#include "flow.h"
#include "utils.h"


static void testPrettifyText()
{
    struct Test {
        const char* original;
        const char* prettified;
    };

    const Test tests[] = {
        // Remove leading whitespace
        {"\n\t\r", ""},
        {"\n\t\ra", "a"},
        // Split ligatures
        {"a\357\254\201b", "afib"},
        {"a\357\254\202b", "aflb"},
        // Remove paragraph consisting of a single space
        {"a\n \n", "a"},
        {"a\n \nb", "ab"},
        {"a\n  \n", "a\n  \n"},
        {"a\n  \nb", "a\n  \nb"},
        // Remove unnecessary trailing newlines
        {"a\n", "a\n"},
        {"a\n\n", "a\n"},
        {"a\n\n\n", "a\n"},
        {"a\n\n\n\n", "a\n"},
    };

    for (const auto& test : tests) {
        std::string str = test.original;
        const auto strLen = dpso::prettifyTesseractText(&str[0]);

        if (std::strcmp(str.c_str(), test.prettified) != 0) {
            std::fprintf(
                stderr,
                "prettifyTesseractText(\"%s\"): "
                "expected \"%s\", got \"%s\"\n",
                test::utils::escapeStr(test.original).c_str(),
                test::utils::escapeStr(test.prettified).c_str(),
                test::utils::escapeStr(str.c_str()).c_str());
            test::failure();
        }

        if (strLen != std::strlen(str.c_str())) {
            std::fprintf(
                stderr,
                "prettifyTesseractText(\"%s\"): "
                "expected string length %zu, got %zu\n",
                test::utils::escapeStr(test.original).c_str(),
                std::strlen(str.c_str()),
                strLen);
            test::failure();
        }
    }
}


static void testTesseractUtils()
{
    testPrettifyText();
}


REGISTER_TEST(testTesseractUtils);
