#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>


namespace dpso::str {


// Python-style brace string formatting. The argument lookup is
// delegated to FindArg, which has the following signature:
//
//   std::optional<std::string_view> findArg(std::string_view name)
//
// FindArg can return nullopt if an argument with the given name is
// not found.
template<typename FindArg>
std::string format(std::string_view fmt, FindArg findArg)
{
    std::string result;

    static const std::string_view braces{"{}"};

    while (true) {
        const auto brace1Pos = fmt.find_first_of(braces);
        if (brace1Pos == fmt.npos)
            break;

        const auto argPos = brace1Pos + 1;
        const auto brace2Pos = fmt.find_first_of(braces, argPos);
        if (brace2Pos == fmt.npos)
            break;

        const auto specEndPos = brace2Pos + 1;

        if (fmt[brace1Pos] == fmt[brace2Pos]) {
            if (argPos != brace2Pos)
                break;

            result.append(fmt, 0, brace2Pos);
        } else if (fmt[brace1Pos] == '{') {
            const std::optional<std::string_view> arg{findArg(
                fmt.substr(argPos, brace2Pos - argPos))};
            if (arg) {
                result.append(fmt, 0, brace1Pos);
                result += *arg;
            } else
                result.append(fmt, 0, specEndPos);
        } else // A }{ pair, possibly with some characters in between.
            break;

        fmt.remove_prefix(specEndPos);
    }

    result.append(fmt);
    return result;
}


}
