#include "plural/tokenizer.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <iterator>

#include "dp_intl/error.h"
#include "str_utils.h"


// See the gettext-runtime/intl/plural.y file from the Gettext source
// code for the grammar.


namespace dp::intl::plural {


std::vector<Token> tokenize(std::string_view s)
{
    std::vector<Token> result;

    while (!s.empty()) {
        if (isBlank(s.front())) {
            s.remove_prefix(1);
            continue;
        }

        {
            const auto* sBegin = s.data();
            const auto* sEnd = sBegin + s.size();

            CountT v;
            const auto [ptr, ec] = std::from_chars(sBegin, sEnd, v);

            if (ec == std::errc{}) {
                result.push_back(
                    {Token::Type::Num, v, s.substr(0, ptr - sBegin)});
                s.remove_prefix(ptr - sBegin);
                continue;
            }

            if (ec == std::errc::result_out_of_range)
                throw Error{
                    "Number " + std::string{sBegin, ptr}
                    + "[...] is out of range"};

            // Doesn't start with a digit. Keep parsing.
            assert(ec == std::errc::invalid_argument);
        }

        using TT = Token::Type;

        static const struct Prefix {
            std::string_view str;
            Token::Type type;
        } prefixes[]{
            // Longer operators come first so that we don't eat "<"
            // before "<=" and so on.
            {"==", TT::Equal},
            {"!=", TT::NotEqual},
            {"<=", TT::LessEqual},
            {">=", TT::GreaterEqual},
            {"&&", TT::And},
            {"||", TT::Or},
            {"n", TT::VarN},
            {"+", TT::Plus},
            {"-", TT::Minus},
            {"*", TT::Star},
            {"/", TT::Slash},
            {"%", TT::Percent},
            {"<", TT::Less},
            {">", TT::Greater},
            {"?", TT::Question},
            {":", TT::Colon},
            {"(", TT::LParen},
            {")", TT::RParen},
            {"!", TT::Not},
        };

        const auto iter = std::find_if(
            std::begin(prefixes), std::end(prefixes),
            [s](const Prefix& prefix)
            {
                return startsWith(s, prefix.str);
            });

        if (iter == std::end(prefixes))
            throw Error{
                "Unknown token starting with \""
                // Include some extra characters, if any, for context.
                + std::string{s, 0, 10}
                + "\""};


        result.push_back(
            {iter->type, 0, s.substr(0, iter->str.size())});
        s.remove_prefix(iter->str.size());
    }

    result.push_back({Token::Type::End, 0, "END"});
    return result;
}


}
