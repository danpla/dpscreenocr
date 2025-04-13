#include <array>

#include "dpso_utils/byte_order.h"
#include "dpso_utils/str.h"

#include "flow.h"
#include "utils.h"


using namespace dpso;


namespace {


const char* toStr(ByteOrder byteOrder)
{
    switch (byteOrder) {
    case ByteOrder::little:
        return "ByteOrder::little";
    case ByteOrder::big:
        return "ByteOrder::big";
    }

    return "";
}


template<typename T>
std::string asHex(T v)
{
    auto s = str::toStr(v, 16);
    s.insert(v < 0 ? 1 : 0, "0x");
    return s;
}


template<ByteOrder byteOrder, typename T>
void testStore(
    T v,
    const std::array<std::uint8_t, sizeof(T)>& expected)
{
    std::array<std::uint8_t, sizeof(T)> result;
    store<byteOrder>(v, result.data());

    if (result != expected)
        test::failure(
            "dpso::store<{}>({}): expected {}, got {}",
            toStr(byteOrder),
            v,
            test::utils::toStr(expected, asHex<std::uint8_t>),
            test::utils::toStr(result, asHex<std::uint8_t>));
}


template<
    ByteOrder byteOrder, typename T, std::size_t numBytes = sizeof(T)>
void testLoad(
    const std::array<std::uint8_t, numBytes>& data,
    T expected)
{
    T result;
    load<byteOrder, numBytes>(result, data.data());
    if (result != expected)
        test::failure(
            "dpso::load<{}, {}>(..., {}): expected {}, got {}",
            toStr(byteOrder),
            numBytes,
            test::utils::toStr(data, asHex<std::uint8_t>),
            asHex(expected),
            asHex(result));
}


void testByteOrder()
{
    testStore<ByteOrder::little, std::uint16_t>(0x0102, {0x02, 0x01});
    testStore<ByteOrder::big, std::uint16_t>(0x0102, {0x01, 0x02});

    testStore<ByteOrder::little, std::int16_t>(-0x3, {0xfd, 0xff});
    testStore<ByteOrder::big, std::int16_t>(-0x3, {0xff, 0xfd});

    testLoad<ByteOrder::little, std::uint16_t>({0x01, 0x02}, 0x0201);
    testLoad<ByteOrder::little, std::uint32_t, 2>(
        {0x01, 0x02}, 0x0201);

    testLoad<ByteOrder::little, std::int16_t>({0xfd, 0xff}, -0x3);
    testLoad<ByteOrder::little, std::int32_t, 2>(
        {0xfd, 0xff}, 0xfffd);

    testLoad<ByteOrder::big, std::uint16_t>({0x01, 0x02}, 0x0102);
    testLoad<ByteOrder::big, std::uint32_t, 2>({0x01, 0x02}, 0x0102);

    testLoad<ByteOrder::big, std::int16_t>({0xff, 0xfd}, -0x3);
    testLoad<ByteOrder::big, std::int32_t, 2>({0xff, 0xfd}, 0xfffd);
}


}


REGISTER_TEST(testByteOrder);

