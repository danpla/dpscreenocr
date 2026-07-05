#pragma once

#include <string>
#include <string_view>
#include <vector>


namespace dp::intl {


/**
 * The language tag format options for buildLangTag().
 *
 * The default LangTagFormat uses BCP 47 style letter case and
 * underscores as separators to match the convention used by the
 * Gettext library.
 */
struct LangTagFormat {
    /**
     * The separator to be inserted between subtags.
     *
     * buildLangTag() is designed to make tags compatible with the
     * matchLang() function, so the separators are intentionally
     * limited to hyphens and underscores.
     */
    enum class Separator {
        Hyphen,
        Underscore
    };

    /**
     * The letter case for the tag.
     *
     * LetterCase::Upper and LetterCase::Lower simply convert each
     * subtag to upper and lowercase, respectively.
     *
     * LetterCase::Bcp47 applies the case convention used by BCP 47
     * language tags, where all subtags are in lowercase with two
     * exceptions: 2-letter and 4-letter subtags that neither appear
     * at the start of the tag nor occur after 1-letter subtags. Such
     * 2-letter subtags are in uppercase, and 4-letter subtags are
     * titlecase.
     */
    enum class LetterCase {
        Upper,
        Lower,
        Bcp47
    };

    Separator separator{Separator::Underscore};
    LetterCase letterCase{LetterCase::Bcp47};
};


/**
 * Build a language tag from individual subtags, normalizing the
 * letter case.
 *
 * You can use this function together with LangMatchFn to build a
 * language tag that use the same separators and letter case as the
 * tags in your project. In particular, this is useful in cases when
 * you cannot perform a case-insensitive string comparison, such as
 * when querying the file system.
 */
std::string buildLangTag(
    const std::vector<std::string_view>& subtags,
    const LangTagFormat& format = {});


}
