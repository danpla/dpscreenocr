
#include <utility>

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
            "line {}: {} != {}",
            lineNum, test::utils::toStr(a), test::utils::toStr(b));
}


#define TEST_EQUAL(a, b) testEqual(a, b, __LINE__)


void testReadWrite()
{
    dpso::StringStream stream{"abcdef"};

    TEST_EQUAL(readSome(stream, 2), "ab");
    dpso::write(stream, "01");

    TEST_EQUAL(readSome(stream, 2), "ef");
    dpso::write(stream, "23");

    TEST_EQUAL(stream.getStr(), "ab01ef23");
    TEST_EQUAL(stream.takeStr(), "ab01ef23");
    TEST_EQUAL(stream.getStr(), "");

    dpso::write(stream, "45");
    TEST_EQUAL(stream.getStr(), "45");
}


void testCopy()
{
    dpso::StringStream stream;
    dpso::write(stream, "a");

    auto stream2{stream};

    TEST_EQUAL(stream.getStr(), "a");
    TEST_EQUAL(readSome(stream, 1), "");
    dpso::write(stream, "b");
    TEST_EQUAL(stream.getStr(), "ab");

    TEST_EQUAL(stream2.getStr(), "a");
    TEST_EQUAL(readSome(stream2, 1), "");
    dpso::write(stream2, "b");
    TEST_EQUAL(stream2.getStr(), "ab");

    auto stream3 = stream2;

    TEST_EQUAL(stream2.getStr(), "ab");
    TEST_EQUAL(readSome(stream2, 1), "");
    dpso::write(stream2, "c");
    TEST_EQUAL(stream2.getStr(), "abc");

    TEST_EQUAL(stream3.getStr(), "ab");
    TEST_EQUAL(readSome(stream3, 1), "");
    dpso::write(stream3, "c");
    TEST_EQUAL(stream3.getStr(), "abc");
}


void testMove()
{
    dpso::StringStream stream;
    dpso::write(stream, "a");

    auto stream2{std::move(stream)};

    TEST_EQUAL(stream.getStr(), "");
    TEST_EQUAL(readSome(stream, 1), "");
    dpso::write(stream, "a2");
    TEST_EQUAL(stream.getStr(), "a2");

    TEST_EQUAL(stream2.getStr(), "a");
    TEST_EQUAL(readSome(stream2, 1), "");
    dpso::write(stream2, "b");
    TEST_EQUAL(stream2.getStr(), "ab");

    auto stream3 = std::move(stream2);

    TEST_EQUAL(stream2.getStr(), "");
    TEST_EQUAL(readSome(stream2, 1), "");
    dpso::write(stream2, "ab2");
    TEST_EQUAL(stream2.getStr(), "ab2");

    TEST_EQUAL(stream3.getStr(), "ab");
    TEST_EQUAL(readSome(stream3, 1), "");
    dpso::write(stream3, "c");
    TEST_EQUAL(stream3.getStr(), "abc");
}


void testStringStream()
{
    testReadWrite();
    testCopy();
    testMove();
}


}


REGISTER_TEST(testStringStream);
