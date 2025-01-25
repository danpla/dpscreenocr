
#include "flow.h"
#include "utils.h"

#include "dpso_utils/os.h"
#include "dpso_utils/stream/out_newline_conversion_stream.h"
#include "dpso_utils/stream/string_stream.h"
#include "dpso_utils/stream/utils.h"


namespace {


void testOutNewlineConversionStream()
{
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
        dpso::StringStream strStream;
        dpso::OutNewlineConversionStream convStream{
            strStream, test.outNewline};

        dpso::write(convStream, input);

        if (strStream.getStr() != test.expected)
            test::failure(
                "Converting {} to {} newlines: expected {}, got {}",
                test::utils::toStr(input),
                test.outNewline
                    ? test::utils::toStr(test.outNewline) : "OS",
                test::utils::toStr(test.expected),
                test::utils::toStr(strStream.getStr()));
    }
}


}


REGISTER_TEST(testOutNewlineConversionStream);
