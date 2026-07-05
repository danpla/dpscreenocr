#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "dp_intl/error.h"
#include "plural/token.h"


namespace dp::intl::plural {


class TokenStream {
public:
    explicit TokenStream(std::vector<Token>&& tokens)
        : tokens{std::move(tokens)}
    {
    }

    const Token& peek() const
    {
        if (pos >= tokens.size())
            throw Error{"Unexpected end of token list"};

        return tokens[pos];
    }

    const Token& consume()
    {
        const auto& result = peek();
        ++pos;
        return result;
    }
private:
    std::vector<Token> tokens;
    std::size_t pos{};
};


}
