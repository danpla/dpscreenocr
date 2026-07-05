/**
 * \file
 * Default PluralSpec implementations for dp::intl::Translator.
 */
#pragma once

#include "dp_intl/count_t.h"


namespace dp::intl {


/**
 * A plural specification for the Germanic family of languages.
 *
 * This specification is suitable for English, Spanish, German and
 * other Germanic languages that use two plural forms: one for 1 (the
 * singular form) and another for all other numbers.
 */
struct GermanicPluralSpec {
    static constexpr CountT numPlurals{2};

    CountT operator()(CountT n) const
    {
        return n == 1 ? 0 : 1;
    }
};


/**
 * A specification for a single plural form.
 *
 * You can use this specification for languages with a single plural
 * form (like Japanese or Korean), or when you prefer an ID-based
 * translation (i.e., when you use short string identifiers instead of
 * embedding translatable text directly into the source code).
 */
struct SingleFormPluralSpec {
    static constexpr CountT numPlurals{1};

    CountT operator()(CountT /*n*/) const
    {
        return 0;
    }
};


}
