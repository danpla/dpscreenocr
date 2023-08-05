
#pragma once

#include "ocr/error.h"


namespace dpso::ocr {


class InvalidLangCodeError : public Error {
    using Error::Error;
};


// Throws InvalidLangCodeError.
void validateLangCode(const char* langCode);


}
