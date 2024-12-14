
#include <cstring>
#include <string>

#include "dpso/ocr/tesseract/utils.h"

#include "flow.h"
#include "utils.h"


namespace {


void testPrettifyText()
{
    const struct {
        const char* original;
        const char* prettified;
    } tests[]{
        // Trim whitespace
        {" \n\t\r", ""},
        {" \n\t\ra", "a"},
        {"a \n\t\r", "a"},
        {" \n\t\ra \n\t\r ", "a"},
        {" \n\t\ra \n\t\r b", "a \n\t\r b"},
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
        const auto strLen = dpso::ocr::tesseract::prettifyText(
            str.data());

        if (std::strcmp(str.c_str(), test.prettified) != 0)
            test::failure(
                "tesseract::prettifyText({}): "
                "expected {}, got {}",
                test::utils::toStr(test.original),
                test::utils::toStr(test.prettified),
                test::utils::toStr(str));

        if (strLen != std::strlen(str.c_str()))
            test::failure(
                "tesseract::prettifyText({}): "
                "expected string length {}, got {}",
                test::utils::toStr(test.original),
                std::strlen(str.c_str()),
                strLen);
    }
}


void testTesseractUtils()
{
    testPrettifyText();
}


}


REGISTER_TEST(testTesseractUtils);
