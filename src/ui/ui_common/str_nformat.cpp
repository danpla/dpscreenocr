#include "str_nformat.h"

#include <optional>

#include "dpso_utils/str_format_core.h"


namespace ui {


std::string strNFormat(
    const char* str, std::initializer_list<StrNFormatArg> args)
{
    return dpso::str::format(
        str,
        [&](std::string_view name) -> std::optional<std::string_view>
        {
            for (const auto& arg : args)
                if (arg.name == name)
                    return arg.str;

            return {};
        });
}


}
