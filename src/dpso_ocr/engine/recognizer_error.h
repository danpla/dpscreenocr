#pragma once

#include "engine/error.h"


namespace dpso::ocr {


class RecognizerError : public Error {
    using Error::Error;
};


}
