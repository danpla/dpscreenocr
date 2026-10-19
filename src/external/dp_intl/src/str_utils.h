#pragma once

#include <charconv>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>


namespace dp::intl {


// Locale-independent versions of routines from <cctype>.
unsigned char toUpper(unsigned char c);
unsigned char toLower(unsigned char c);
bool isBlank(unsigned char c);
bool isAlpha(unsigned char c);
bool isDigit(unsigned char c);


template<typename Predicate>
std::string_view trimLeft(std::string_view s, Predicate predicate)
{
    std::size_t n{};
    while (n < s.size() && predicate(s[n]))
        ++n;
    return s.substr(n);
}


template<typename Predicate>
std::string_view trimRight(std::string_view s, Predicate predicate)
{
    auto n = s.size();
    while (n > 0 && predicate(s[n - 1]))
        --n;
    return s.substr(0, n);
}


template<typename Predicate>
std::string_view trim(std::string_view s, Predicate predicate)
{
    return trimLeft(trimRight(s, predicate), predicate);
}


bool lessIgnoreCase(std::string_view a, std::string_view b);
bool equalIgnoreCase(std::string_view a, std::string_view b);


bool startsWith(std::string_view s, std::string_view other);


// libstdc++ versions before 10.1 erroneously reject char16_t and
// char32_t from to_chars() via type traits. As a workaround, we cast
// these types to integers using std::make_unsigned.
template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
std::to_chars_result toChars(char* begin, char* end, T v, int base)
{
    using T2 = std::conditional_t<
        std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>,
        std::make_unsigned_t<T>,
        T>;
    return std::to_chars(begin, end, static_cast<T2>(v), base);
}


// Locale-independent std::to_string().
template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
std::string toStr(T v)
{
    char buf[
        // +1 for numeric_limits::digits10, which is always 1 less
        // than the actual number of digits in the maximum value. For
        // example, digits10 is 2 for a 8-bit unsigned, because it can
        // represent any two-digit number, but not 256-999.
        std::numeric_limits<T>::digits10 + 1
        + std::numeric_limits<T>::is_signed];
    return {buf, toChars(buf, buf + sizeof(buf), v, 10).ptr};
}


template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
std::string toHex(T v)
{
    char buf[sizeof(T) * 2 + std::numeric_limits<T>::is_signed];
    return {buf, toChars(buf, buf + sizeof(buf), v, 16).ptr};
}


std::string toHex(const void* data, std::size_t size);


}
