#pragma once

#include "ocr/error.h"


namespace dpso::ocr {


class LangManagerError : public Error {
    using Error::Error;
};


// The connection either could not be established or was terminated.
// This error is intended as a hint to show the end user a more
// friendly error message, such as "check your network connection and
// try again".
class LangManagerNetworkConnectionError : public LangManagerError {
    using LangManagerError::LangManagerError;
};


}
