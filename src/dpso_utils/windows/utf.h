
#pragma once

#include <string>
#include <stdexcept>


namespace dpso::windows {


class CharConversionError : public std::runtime_error {
    using runtime_error::runtime_error;
};


// Convert UTF-8 to UTF-16. Throws CharConversionError on invalid
// UTF-8 sequence.
std::wstring utf8ToUtf16(const char* utf8Str);


// Convert UTF-16 to UTF-8. Throws CharConversionError on invalid
// UTF-16 sequence
std::string utf16ToUtf8(const wchar_t* utf16Str);


}
