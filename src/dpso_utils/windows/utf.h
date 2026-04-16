#pragma once

#include <string>
#include <string_view>

#include "dpso_utils/os_error.h"


namespace dpso::windows {


class CharConversionError : public os::Error {
    using Error::Error;
};


// Throws CharConversionError on invalid UTF-8 sequence.
std::wstring utf8ToUtf16(std::string_view utf8Str);


// Throws CharConversionError on invalid UTF-16 sequence.
std::string utf16ToUtf8(std::wstring_view utf16Str);


}
