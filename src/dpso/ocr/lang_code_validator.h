
#pragma once

#include "ocr/error.h"


namespace dpso::ocr {


class LangCodeError : public Error {
    using Error::Error;
};


// Throws LangCodeError.
void validateLangCode(const char* langCode);


}
