#pragma once

#include <cstddef>
#include <type_traits>


namespace dp::intl::plural {


template<typename T>
class TokenTypeMap {
public:
    constexpr const T& operator[](Token::Type type) const
    {
        return a[getIdx(type)];
    }

    constexpr T& operator[](Token::Type type)
    {
        return a[getIdx(type)];
    }
private:
    static_assert(
        std::is_unsigned_v<std::underlying_type_t<Token::Type>>);

    static constexpr std::size_t getIdx(Token::Type type)
    {
        return static_cast<std::size_t>(type);
    }

    T a[getIdx(Token::Type::End) + 1]{};
};


}
