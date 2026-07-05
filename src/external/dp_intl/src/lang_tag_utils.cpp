#include "dp_intl/lang_tag_utils.h"

#include <cstddef>

#include "str_utils.h"


namespace dp::intl {
namespace {


enum class LetterCase {
    Upper,
    Lower,
    Title
};


void append(
    std::string& result, std::string_view str, LetterCase letterCase)
{
    result.reserve(result.size() + str.size());

    switch (letterCase) {
    case LetterCase::Upper:
        for (auto c : str)
            result += toUpper(c);
        break;
    case LetterCase::Lower:
        for (auto c : str)
            result += toLower(c);
        break;
    case LetterCase::Title:
        if (str.empty())
            break;

        result += toUpper(str[0]);
        for (std::size_t i{1}; i < str.size(); ++i)
            result += toLower(str[i]);
        break;
    };
}


}


std::string buildLangTag(
    const std::vector<std::string_view>& subtags,
    const LangTagFormat& format)
{
    std::string tag;

    const auto sep =
        format.separator == LangTagFormat::Separator::Hyphen
        ? '-' : '_';

    bool wasSignleton{};
    for (std::size_t i{}; i < subtags.size(); ++i) {
        if (i > 0)
            tag += sep;

        const auto subtag = subtags[i];
        if (subtag.size() == 1)
            wasSignleton = true;

        LetterCase letterCase{};
        switch (format.letterCase) {
        case LangTagFormat::LetterCase::Lower:
            letterCase = LetterCase::Lower;
            break;
        case LangTagFormat::LetterCase::Upper:
            letterCase = LetterCase::Upper;
            break;
        case LangTagFormat::LetterCase::Bcp47:
            letterCase = LetterCase::Lower;

            if (i > 0 && !wasSignleton) {
                if (subtag.size() == 4)
                    // Script
                    letterCase = LetterCase::Title;
                else if (subtag.size() == 2)
                    // Region
                    letterCase = LetterCase::Upper;
            }
            break;
        }

        append(tag, subtag, letterCase);
    }

    return tag;
}


}
