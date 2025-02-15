#include "engine/lang_code_validator.h"

#include "dpso_utils/str.h"


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
        throw LangCodeError{"Language code is empty"};

    for (const auto* s = langCode; *s; ++s)
        if (!isValidLangCodeChar(*s))
            throw LangCodeError{str::format(
                "Invalid character at index {}", langCode - s)};
}


}
