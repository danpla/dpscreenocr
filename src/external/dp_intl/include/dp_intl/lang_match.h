#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>


namespace dp::intl {


/**
 * The language tag callback of matchLang().
 *
 * The function returns `true` if one of your languages matches the
 * given language tag (represented as a list of individual subtags).
 *
 * Generally, you should use case-insensitive string comparison when
 * matching tags or any strings based on tags, such as file names. If
 * a case-insensitive comparison is not possible, such as when
 * querying the file system to check if a file exists, you must
 * normalize the tags (or file names, etc.) to the same letter case
 * (for example, by using buildLangTag()).
 */
using LangMatchFn = std::function<bool(
    const std::vector<std::string_view>& subtags)>;


/**
 * Choose a language based on the priority list.
 *
 * The purpose of the function is to help you pick the most suitable
 * language based on the language priority list provided by the user.
 *
 *
 * Language tag format
 * ===================
 *
 * Each tag in the priority list has the format similar to BCP 47
 * ([RFC 5646]) and locale identifiers on most platforms. The tag
 * consists of one or more subtags divided by separators. A subtag is
 * one or more alphanumeric characters (A-Z, a-z, 0-9), a separator is
 * either an underscore or hyphen.
 *
 * [RFC 5646]: https://www.rfc-editor.org/info/rfc5646/
 *
 * In its basic (and most common) form, a language tag consists of:
 *
 * * The primary language subtag, such as `en` for English or `zh` for
 *   Chinese.
 *
 * * An optional 4-letter script subtag, such as `Latn` for the Latin
 *   script or `Hans` for the Simplified Chinese script.
 *
 * * An optional 2-letter region subtag, such as `GB` for Great
 *   Britain or `CN` for Mainland China.
 *
 * Examples of language tags include:
 *
 * * `en` for English. This can imply either American or British
 *   English, depending on the context.
 *
 * * `en-GB` for British English.
 *
 * * `zh-Hans` for Chinese written in the Simplified script.
 *
 * * `zh-CN` for Chinese used in Mainland China. This usually implies
 *   the Simplified script as if in the explicit `zh-Hans-CN` tag.
 *
 * * `zh-Hant-HK` for Chinese, written in the Traditional script, as
 *   used in Hong Kong.
 *
 * When matchLang() parses a tag string, it tries to extract as much
 * subtags as possible, stopping either when the end of the string is
 * reached, or when one of the following occurs:
 *
 * * A single-character subtag. Such subtags are called "singletons"
 *   in BCP 47.
 *
 * * An empty subtag. Such a subtag can appear as a leading/trailing
 *   subtag separator, or two separators in the middle of the tag.
 *
 * * A symbol that is not a valid subtag or separator character.
 *
 *   Note that this will effectively strip any platform-specific
 *   "extensions" from locale identifiers, such as `.codeset` and
 *   `@modifier` used in POSIX platforms. For example,
 *   `de_DE.UTF-8@euro` is treated the same as `de_DE`.
 *
 *
 * Matching algorithm
 * ==================
 *
 * For each language tag in the priority list, matchLang() generates
 * the tag variants using the algorithm described below, and calls
 * `matchFn` for each variant until the function either returns `true`
 * (indicating a successful match) or throws an exception.
 *
 * `matchFn` will never be called more than once for the same tag. The
 * tag comparison is case-insensitive, but be aware that the case of
 * the tag passed to `matchFn` will be the same as in the original tag
 * string that was encountered first among the possible duplicates.
 *
 * Note that for `matchFn`, a tag is represented as a list of subtags
 * rather than a single string. For simplicity, the rest of this text
 * will denote these lists using the full tag strings, as if subtags
 * were joined by hyphens.
 *
 * Basic tag variants
 * ------------------
 *
 * matchLang() splits a tag into the list of sutags, and performs the
 * following steps, repeatedly:
 *
 * 1. If the current subtag list is empty, the lookup for this tag
 *    ends.
 *
 * 2. `matchFn` is called with the current subtag list.
 *
 * 3. If there are more than 2 subtags in the current list, `matchFn`
 *    is called with a series of additional lists, where each
 *    subsequent list is a version of the previous list with the
 *    penultimate tag removed. The series ends with a 2-item list
 *    consisting of the first and last subtags of the current list.
 *
 * 4. The last subtag is removed from the current list and algorithm
 *    continues from the step 1.
 *
 * For example, if we have a tag consisting of 4 subtags, the full
 * sequence for `matchFn` would be as follows. Here, numbers refer to
 * the subtag positions in the initial tag.
 *
 *     1 2 3 4
 *     1 2 4
 *     1 4
 *     1 2 3
 *     1 3
 *     1 2
 *     1
 *
 * Additional tag variants
 * -----------------------
 *
 * During execution of the basic algorithm described above,
 * matchLang() can generate additional tag variants to handle forward
 * and backward compatibility between the naming of you languages and
 * the tags from the priority list.
 *
 * The additional tags are not subject to the transformations
 * performed by the tag variant generation algorithm described in the
 * previous section. Instead, these tags are injected into it when
 * `matchFn` returns `false` for a basic tag.
 *
 * Currently, the additional variants are generated for Chinese
 * language tags (the first subtag is `zh`). The problem with Chinese
 * language tags is that, historically, people used `zh-CN` (Chinese
 * used in Mainland China) to imply Simplified Chinese, and `zh-TW`
 * (Chinese used in Taiwan) to imply Traditional Chinese. This
 * practice was deprecated in favor of the `zh-Hans` and `zh-Hant`
 * tags, which explicitly specify the script rather than implying it
 * by the region subtag. For more information, see
 * [Using Hans and Hant codes].
 *
 * [Using Hans and Hant codes]: https://www.w3.org/International/geo/html-tech/tech-lang.html#ri20040429.113217290
 *
 * Depending on the source of the language priority list, the Chinese
 * tags can be in either the old or the new format. To unify the
 * language lookup, matchLang() does the following:
 *
 * * If `matchFn` returns `false` for `zh-Hans` (Simplified Chinese),
 *   it will be called again with `zh-CN` and `zh-SG`, in that order.
 *   Similarly, the additional tags for `zh-Hant` (Traditional
 *   Chinese) are `zh-TW` and `zh-HK`.
 *
 * * Conversely, for any old-style tag denoting Simplified Chinese
 *   (e.g. `zh-CN`) or Traditional Chinese (e.g., `zh-TW`), `matchFn`
 *   will also be called with `zh-Hans` or `zh-Hant`, respectively.
 */
void matchLang(
    const std::vector<std::string>& langTagPriorityList,
    const LangMatchFn& matchFn);


}
