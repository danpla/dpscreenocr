#pragma once

#include <cstddef>
#include <type_traits>


namespace dpso {


enum class ByteOrder {
    little,
    big
};


template<ByteOrder byteOrder, std::size_t n, typename Fn>
void iLoop(Fn fn)
{
    if (byteOrder == ByteOrder::little)
        for (std::size_t i = 0; i < n; i++)
            fn(i);
    else
        for (auto i = n; i--;)
            fn(i);
}


template<ByteOrder byteOrder, typename T>
void store(T v, void* data)
{
    const auto uv = static_cast<std::make_unsigned_t<T>>(v);
    auto* dst = static_cast<unsigned char*>(data);

    iLoop<byteOrder, sizeof(uv)>(
        [&](std::size_t i)
        {
            *dst++ = (uv >> (i * 8)) & 0xff;
        });
}


template<ByteOrder byteOrder, std::size_t numBytes, typename T>
void load(T& v, const void* data)
{
    static_assert(numBytes <= sizeof(T));

    using UT = std::make_unsigned_t<T>;
    const auto* src = static_cast<const unsigned char*>(data);

    UT uv{};

    iLoop<byteOrder, numBytes>(
        [&](std::size_t i)
        {
            uv |= static_cast<UT>(*src++) << (i * 8);
        });

    v = static_cast<T>(uv);
}


template<ByteOrder byteOrder, typename T>
void load(T& v, const void* data)
{
    load<byteOrder, sizeof(T), T>(v, data);
}


}
