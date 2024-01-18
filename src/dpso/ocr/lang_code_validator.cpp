
#include "ocr/lang_code_validator.h"

#include <fmt/core.h>


namespace dpso::ocr {


static bool isValidLangCodeChar(char c)
{
    return
        (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || c == '.'
        || c == '-'
        || c == '_';
}


void validateLangCode(const char* langCode)
{
    if (!*langCode)
        throw InvalidLangCodeError{"Language code is empty"};

    for (const auto* s = langCode; *s; ++s)
        if (!isValidLangCodeChar(*s))
            throw InvalidLangCodeError{fmt::format(
                "Invalid character at position {}", langCode - s)};
}


}
