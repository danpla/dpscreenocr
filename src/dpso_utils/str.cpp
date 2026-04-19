#include "str.h"

#include <algorithm>
#include <charconv>
#include <climits>
#include <cstdint>
#include <optional>

#include "str_format_core.h"
#include "str_stdio.h"


namespace dpso::str {


bool isBlank(unsigned char c)
{
    return c == '\t' || c == ' ';
}


bool isSpace(unsigned char c)
{
    static const std::string_view spaces{" \f\n\r\t\v"};
    return spaces.find(c) != spaces.npos;
}


static unsigned char toLower(unsigned char c)
{
    if (c < 'A' || c > 'Z')
        return c;

    return c + ('a' - 'A');
}


static int cmpIgnoreCase(
    std::string_view a, std::string_view b, std::size_t len)
{
    for (std::size_t i{}; i < len; ++i)
        if (const auto d = toLower(a[i]) - toLower(b[i]); d != 0)
            return d;

    return 0;
}


int cmpIgnoreCase(std::string_view a, std::string_view b)
{
    const auto r = cmpIgnoreCase(a, b, std::min(a.size(), b.size()));
    if (r != 0)
        return r;

    if (a.size() < b.size())
        return -1;

    if (a.size() > b.size())
        return 1;

    return 0;
}


bool equalIgnoreCase(std::string_view a, std::string_view b)
{
    return a.size() == b.size()
        && cmpIgnoreCase(a, b, a.size()) == 0;
}


bool startsWith(std::string_view s, std::string_view other)
{
    return s.size() >= other.size()
        && s.compare(0, other.size(), other) == 0;
}


bool endsWith(std::string_view s, std::string_view other)
{
    return s.size() >= other.size()
        && s.compare(
            s.size() - other.size(), other.size(), other) == 0;
}


std::string_view trimLeft(
    std::string_view s, bool (&pred)(unsigned char c))
{
    std::size_t n{};
    while (n < s.size() && pred(s[n]))
        ++n;
    return s.substr(n);
}


std::string_view trimRight(
    std::string_view s, bool (&pred)(unsigned char c))
{
    auto n = s.size();
    while (n > 0 && pred(s[n - 1]))
        --n;
    return s.substr(0, n);
}


std::string_view trim(
    std::string_view s, bool (&pred)(unsigned char c))
{
    return trimLeft(trimRight(s, pred), pred);
}


std::string justifyLeft(std::string s, std::size_t width, char fill)
{
    if (s.size() < width)
        s.resize(width, fill);

    return s;
}


std::string justifyRight(std::string s, std::size_t width, char fill)
{
    if (s.size() < width)
        s.insert(0, width - s.size(), fill);

    return s;
}


std::string toStr(char c)
{
    return std::string(1, c);
}


template<typename T>
static std::string intToStr(T v, int base)
{
    // Reserve space for the base-2 representation and a minus sign.
    char buf[sizeof(T) * CHAR_BIT + 1];
    const auto r = std::to_chars(buf, buf + sizeof(buf), v, base);
    // to_chars() can only fail with std::errc::value_too_large.
    return {buf, r.ptr};
}


std::string toStr(int v, int base)
{
    return intToStr(v, base);
}


std::string toStr(unsigned v, int base)
{
    return intToStr(v, base);
}


std::string toStr(long v, int base)
{
    return intToStr(v, base);
}


std::string toStr(unsigned long v, int base)
{
    return intToStr(v, base);
}


std::string toStr(long long v, int base)
{
    return intToStr(v, base);
}


std::string toStr(unsigned long long v, int base)
{
    return intToStr(v, base);
}


// At the time of writing, not all compilers have support for the
// floating-point std::to_chars overloads, so we use std::to_string
// and manually replace a decimal comma (if emitted by the current
// locale) with a period.
static std::string doubleToStr(double v)
{
    auto result = std::to_string(v);

    const auto lastNonzero = result.find_last_not_of('0');
    const auto sep = result.find_last_of(".,", lastNonzero);

    if (sep == result.npos)
        return result;

    if (sep == lastNonzero) {
        // Only zeros after the decimal separator.
        result.resize(sep);
        return result;
    }

    result[sep] = '.';

    if (lastNonzero != result.npos)
        result.resize(lastNonzero + 1);

    return result;
}


std::string toStr(float v)
{
    return doubleToStr(v);
}


std::string toStr(double v)
{
    return doubleToStr(v);
}


std::string toHex(const void* data, std::size_t size)
{
    std::string result;
    result.reserve(size * 2);

    const auto* bytes = static_cast<const std::uint8_t*>(data);
    static const auto* hexChars = "0123456789abcdef";

    for (std::size_t i{}; i < size; ++i) {
        const auto b = bytes[i];
        result += hexChars[b >> 4];
        result += hexChars[b & 0x0f];
    }

    return result;
}


static auto makeArgLookupFn(
    std::initializer_list<std::string_view> args)
{
    return
        [iter = args.begin(), end = args.end()]
        (std::string_view name) mutable
        -> std::optional<std::string_view>
        {
            if (name.empty() && iter < end)
                return *iter++;

            return {};
        };
}


std::string format(
    std::string_view fmt,
    std::initializer_list<std::string_view> args)
{
    return format(fmt, makeArgLookupFn(args));
}


void print(
    std::FILE* fp,
    std::string_view fmt,
    std::initializer_list<std::string_view> args)
{
    return format(
        fmt,
        makeArgLookupFn(args),
        [&](std::string_view str)
        {
            std::fwrite(str.data(), 1, str.size(), fp);
        });
}


}
