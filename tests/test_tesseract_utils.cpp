
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
        // Trim whitespace
        {"\n\t\r", ""},
        {"\n\t\ra", "a"},
        {"a\n\t\r", "a"},
        {"\n\t\ra\n\t\r", "a"},
        // Split ligatures
        {"a\357\254\201b", "afib"},
        {"a\357\254\202b", "aflb"},
        // Remove paragraph consisting of a single space
        {"a\n\nb", "a\n\nb"},
        {"a\n \nb", "ab"},
        {"a\n  \nb", "a\n  \nb"},
    };

    for (const auto& test : tests) {
        std::string str = test.original;
        const auto strLen = dpso::prettifyTesseractText(&str[0]);

        if (std::strcmp(str.c_str(), test.prettified) != 0)
            test::failure(
                "prettifyTesseractText(\"%s\"): "
                "expected \"%s\", got \"%s\"\n",
                test::utils::escapeStr(test.original).c_str(),
                test::utils::escapeStr(test.prettified).c_str(),
                test::utils::escapeStr(str.c_str()).c_str());

        if (strLen != std::strlen(str.c_str()))
            test::failure(
                "prettifyTesseractText(\"%s\"): "
                "expected string length %zu, got %zu\n",
                test::utils::escapeStr(test.original).c_str(),
                std::strlen(str.c_str()),
                strLen);
    }
}


static void testTesseractUtils()
{
    testPrettifyText();
}


REGISTER_TEST(testTesseractUtils);
