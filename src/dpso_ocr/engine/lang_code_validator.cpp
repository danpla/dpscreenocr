#include "engine/lang_code_validator.h"

#include <cstddef>

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


void validateLangCode(std::string_view langCode)
{
    if (langCode.empty())
        throw LangCodeError{"Language code is empty"};

    for (std::size_t i{}; i < langCode.size(); ++i)
        if (!isValidLangCodeChar(langCode[i]))
            throw LangCodeError{str::format(
                "Invalid character at index {}", i)};
}


}
