#pragma once

#include <charconv>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>


namespace dp::intl {


unsigned char toUpper(unsigned char c);
unsigned char toLower(unsigned char c);

bool isBlank(unsigned char c);
std::string_view trimBlanks(std::string_view s);

bool lessIgnoreCase(std::string_view a, std::string_view b);
bool equalIgnoreCase(std::string_view a, std::string_view b);

bool startsWith(std::string_view s, std::string_view other);


// Locale-independent std::to_string().
template<
    typename T,
    std::enable_if_t<std::numeric_limits<T>::is_integer, int> = 0>
std::string toStr(T v)
{
    char buf[
        // +1 for numeric_limits::digits10, which is always 1 less
        // than the actual number of digits in the maximum value. For
        // example, digits10 is 2 for a 8-bit unsigned, because it can
        // represent any two-digit number, but not 256-999.
        std::numeric_limits<T>::digits10 + 1
        // +1 if signed for a possible minus sign.
        + std::numeric_limits<T>::is_signed];
    const auto r = std::to_chars(buf, buf + sizeof(buf), v, 10);
    // to_chars() can only fail with std::errc::value_too_large.
    return {buf, r.ptr};
}


}
