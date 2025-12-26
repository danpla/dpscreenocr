#include <optional>

#include "flow.h"
#include "utils.h"

#include "dpso_utils/os.h"
#include "dpso_utils/stream/file_stream.h"
#include "dpso_utils/stream/out_newline_conversion_stream.h"
#include "dpso_utils/stream/utils.h"


namespace {


void testOutNewlineConversionStream()
{
    const auto* fileName = "test_out_newline_conversion_stream.txt";
    const auto* input = " \n \r \r\n ";

    const struct {
        const char* outNewline;
        std::string expected;
    } tests[]{
        {nullptr, test::utils::lfToNativeNewline(input)},
        {"", "  \r \r "},
        {"\n", input},
        {"abc", " abc \r \rabc "},
    };

    for (const auto& test : tests) {
        std::optional<dpso::FileStream> file;
        try {
            file.emplace(fileName, dpso::FileStream::Mode::write);
        } catch (dpso::os::Error& e) {
            test::fatalError(
                "FileStream(\"{}\", Mode::write): {}",
                fileName, e.what());
        }

        dpso::OutNewlineConversionStream convStream{
            *file, test.outNewline};

        dpso::write(convStream, input);

        file.reset();
        const auto actual = test::utils::loadText(
            "testOutNewlineConversionStream", fileName);

        if (actual != test.expected)
            test::failure(
                "Converting {} to {} newlines: expected {}, got {}",
                test::utils::toStr(input),
                test.outNewline
                    ? test::utils::toStr(test.outNewline) : "OS",
                test::utils::toStr(test.expected),
                test::utils::toStr(actual));
    }

    test::utils::removeFile(fileName);
}


}


REGISTER_TEST(testOutNewlineConversionStream);
