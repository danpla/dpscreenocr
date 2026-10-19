#include "dp_intl/lang_tag_utils.h"

#include <cstddef>

#include "str_utils.h"


namespace dp::intl {


std::string buildLangTag(
    const std::vector<std::string_view>& subtags,
    const LangTagFormat& format)
{
    std::string tag;

    const auto append = [&tag](std::string_view subtag, auto caseFn)
    {
        tag.reserve(tag.size() + subtag.size());
        for (auto c : subtag)
            tag += caseFn(c);
    };

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

        switch (format.letterCase) {
        case LangTagFormat::LetterCase::Lower:
            append(subtag, toLower);
            break;
        case LangTagFormat::LetterCase::Upper:
            append(subtag, toUpper);
            break;
        case LangTagFormat::LetterCase::Bcp47:
            if (i > 0 && !wasSignleton) {
                if (subtag.size() == 4) {
                    // Script
                    append(subtag.substr(0, 1), toUpper);
                    append(subtag.substr(1), toLower);
                    break;
                } else if (subtag.size() == 2) {
                    // Region
                    append(subtag, toUpper);
                    break;
                }
            }

            append(subtag, toLower);
            break;
        }
    }

    return tag;
}


}
