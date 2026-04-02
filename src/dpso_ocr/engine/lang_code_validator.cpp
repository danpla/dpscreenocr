#include "engine/lang_code_validator.h"

#include <algorithm>

#include "dpso_utils/str.h"


namespace dpso::ocr {


static bool isValidChar(char c)
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

    if (const auto iter = std::find_if_not(
                langCode.begin(), langCode.end(), isValidChar);
            iter != langCode.end())
        throw LangCodeError{str::format(
            "Invalid character '{}' at index {}",
            *iter, iter - langCode.begin())};
}


}
