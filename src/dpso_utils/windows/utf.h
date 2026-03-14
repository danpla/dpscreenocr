#pragma once

#include <string>
#include <string_view>
#include <stdexcept>


namespace dpso::windows {


class CharConversionError : public std::runtime_error {
    using runtime_error::runtime_error;
};


// Throws CharConversionError on invalid UTF-8 sequence.
std::wstring utf8ToUtf16(std::string_view utf8Str);


// Throws CharConversionError on invalid UTF-16 sequence.
std::string utf16ToUtf8(std::wstring_view utf16Str);


}
