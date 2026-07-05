#pragma once

#include <string_view>

#include "dp_intl/count_t.h"


namespace dp::intl::plural {


struct Token {
    enum class Type : unsigned {
        Num,
        VarN,
        Plus,
        Minus,
        Star,
        Slash,
        Percent,
        Equal,
        NotEqual,
        Less,
        LessEqual,
        Greater,
        GreaterEqual,
        And,
        Or,
        Question,
        Colon,
        LParen,
        RParen,
        Not,
        End
    };

    Type type;
    CountT val;

    // The token source string as it appears in the expression, or
    // "End" for the End token.
    std::string_view str;
};


}
