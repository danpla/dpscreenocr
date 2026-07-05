#pragma once

#include <optional>
#include <string_view>

#include "dp_intl/count_t.h"


namespace dp::intl {


// Parser for a GNU-style header entry from MO files.
// www.gnu.org/software/gettext/manual/html_node/Header-Entry.html
struct GnuHeader {
    // Content of the "charset" entry of the "Content-Type" filed.
    // Empty string if "Content-Type" is missing.
    std::string_view charset;

    // Content of the "Plural-Forms" field.
    struct PluralForms {
        CountT nplurals;
        std::string_view plural;
    };
    std::optional<PluralForms> pluralForms;

    // Throws dp::intl::Error.
    static GnuHeader parse(std::string_view s);
};


}
