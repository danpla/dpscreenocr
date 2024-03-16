
#include "flow.h"
#include "utils.h"

#include "dpso_utils/stream/string_stream.h"
#include "dpso_utils/stream/utils.h"


namespace {


std::string readSome(dpso::Stream& stream, std::size_t maxRead)
{
    std::string result(maxRead, ' ');
    result.resize(stream.readSome(result.data(), result.size()));
    return result;
}


void testEqual(
    const std::string& a, const char* b, int lineNum)
{
    if (a != b)
        test::failure(
            "line {}: {} != {}\n",
            lineNum, test::utils::toStr(a), test::utils::toStr(b));
}


#define TEST_EQUAL(a, b) testEqual(a, b, __LINE__)


void testStringStream()
{
    dpso::StringStream stream{"abcdef"};

    TEST_EQUAL(readSome(stream, 2), "ab");
    dpso::write(stream, "12");

    TEST_EQUAL(readSome(stream, 2), "ef");
    dpso::write(stream, "34");

    TEST_EQUAL(stream.getStr(), "ab12ef34");
    TEST_EQUAL(stream.takeStr(), "ab12ef34");
    TEST_EQUAL(stream.getStr(), "");

    dpso::write(stream, "56");
    TEST_EQUAL(stream.getStr(), "56");
}


}


REGISTER_TEST(testStringStream);
