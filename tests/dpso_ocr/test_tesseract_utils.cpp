#include <string>
#include <string_view>

#include "dpso_ocr/engine/tesseract/utils.h"

#include "flow.h"
#include "utils.h"


namespace {


void testPrettifyText()
{
    const struct {
        std::string_view original;
        std::string_view prettified;
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
        std::string str{test.original};

        const std::string_view prettified{
            str.data(),
            dpso::ocr::tesseract::prettifyText(
                str.data())};

        if (prettified != test.prettified)
            test::failure(
                "tesseract::prettifyText({}): "
                "expected {}, got {}",
                test::utils::toStr(test.original),
                test::utils::toStr(test.prettified),
                test::utils::toStr(prettified));
    }
}


void testTesseractUtils()
{
    testPrettifyText();
}


}


REGISTER_TEST(testTesseractUtils);
