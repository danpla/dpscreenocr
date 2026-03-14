#pragma once

#include <string_view>

#include "engine/error.h"


namespace dpso::ocr {


class LangCodeError : public Error {
    using Error::Error;
};


// Throws LangCodeError.
void validateLangCode(std::string_view langCode);


}
