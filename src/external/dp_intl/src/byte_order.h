#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>


namespace dp::intl {


enum class ByteOrder {
    Little,
    Big
};


template<ByteOrder byteOrder, std::size_t n, typename Fn>
constexpr void byteOrderLoop(Fn fn)
{
    if (byteOrder == ByteOrder::Little)
        for (std::size_t i{}; i < n; i++)
            fn(i);
    else
        for (auto i = n; i--;)
            fn(i);
}


template<ByteOrder byteOrder, typename T>
T load(const void* data)
{
    using UT = std::make_unsigned_t<T>;
    const auto* src = static_cast<const std::uint8_t*>(data);

    UT uv{};

    byteOrderLoop<byteOrder, sizeof(T)>(
        [&](std::size_t i)
        {
            uv |= static_cast<UT>(*src++) << (i * 8);
        });

    return static_cast<T>(uv);
}


}
