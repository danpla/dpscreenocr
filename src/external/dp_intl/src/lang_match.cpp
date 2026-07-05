#include "dp_intl/lang_match.h"

#include <algorithm>

#include "str_utils.h"


namespace dp::intl {
namespace {


using Subtags = std::vector<std::string_view>;


void splitTag(std::string_view tag, Subtags& subtags)
{
    const auto isSubtagChar = [](unsigned char c)
    {
        return
            (c >= '0' && c <= '9')
            || (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z');
    };

    const auto isSep = [](unsigned char c)
    {
        return c == '-' || c == '_';
    };

    subtags.clear();

    while (true) {
        const auto subtag = tag.substr(
            0,
            std::find_if_not(tag.begin(), tag.end(), isSubtagChar)
                - tag.begin());
        if (subtag.size() < 2)
            // Either en empty subtag or a single-letter subtag
            // that, according to BCP 47, starts either extension
            // subtags or private use subtags.
            break;

        subtags.push_back(subtag);
        tag.remove_prefix(subtag.size());

        if (tag.empty() || !isSep(tag[0]))
            break;

        tag.remove_prefix(1);
    }
}


class Matcher {
    const LangMatchFn& matchFn;
    std::vector<Subtags> seen;
public:
    explicit Matcher(const LangMatchFn& matchFn)
        : matchFn{matchFn}
    {
    }

    bool operator()(const Subtags& subtags)
    {
        const auto iter = std::lower_bound(
            seen.begin(), seen.end(), subtags,
            [](const Subtags& a, const Subtags& b)
            {
                return std::lexicographical_compare(
                    a.begin(), a.end(), b.begin(), b.end(),
                    lessIgnoreCase);
            });

        if (iter != seen.end()
            && std::equal(
                iter->begin(), iter->end(),
                subtags.begin(), subtags.end(),
                equalIgnoreCase))
            return false;

        if (matchFn(subtags))
            // A successful match stops the entire lookup process, so,
            // as a small optimization, don't add sub-tags to the list
            // of seen entries in this case.
            return true;

        seen.insert(iter, subtags);
        return false;
    }
};


bool matchChineseSpecialCases(
    Matcher& matcher, const Subtags& subtags)
{
    if (subtags.size() != 2 || !equalIgnoreCase(subtags[0], "zh"))
        return false;

    // See:
    // https://www.w3.org/International/geo/html-tech/tech-lang.html#ri20040429.113217290
    // https://www.unicode.org/cldr/charts/48/supplemental/likely_subtags.html
    static const struct {
        std::string_view script;
        // The most commonly used region subtags for the script come
        // first in the list.
        Subtags regions;
    } alternatives[]{
        {"Hans", {"CN", "SG"}}, {"Hant", {"TW", "HK"}},
    };

    // Script -> region.
    if (subtags[1].size() == 4)
        for (const auto& alt : alternatives) {
            if (!equalIgnoreCase(alt.script, subtags[1]))
                continue;

            Subtags variant{subtags[0], {}};
            for (const auto& region : alt.regions) {
                variant[1] = region;
                if (matcher(variant))
                    return true;
            }

            return false;
        }
    // Region -> script.
    else if (subtags[1].size() == 2)
        for (const auto& alt : alternatives)
            for (const auto& region : alt.regions)
                if (equalIgnoreCase(region, subtags[1]))
                    return matcher({subtags[0], alt.script});

    return false;
}


bool matchVariant(Matcher& matcher, const Subtags& subtags)
{
    return
        matcher(subtags) ||
        matchChineseSpecialCases(matcher, subtags);
}


}


void matchLang(
    const std::vector<std::string>& langTagPriorityList,
    const LangMatchFn& matchFn)
{
    if (!matchFn)
        return;

    auto match =
    [
        matcher = Matcher{matchFn},
        subtags = Subtags{},
        variants = Subtags{}]
    (std::string_view tag) mutable
    {
        splitTag(tag, subtags);

        for (; !subtags.empty(); subtags.pop_back()) {
            if (matchVariant(matcher, subtags))
                return true;

            if (subtags.size() > 2)
                for (variants = subtags; variants.size() > 2;) {
                    variants.erase(variants.end() - 2);
                    if (matchVariant(matcher, variants))
                        return true;
                }
        }

        return false;
    };

    for (const auto& tag : langTagPriorityList)
        if (match(tag))
            break;
}


}
