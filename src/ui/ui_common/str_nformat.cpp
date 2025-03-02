#include "str_nformat.h"

#include "dpso_utils/str_format_core.h"


namespace ui {


std::string strNFormat(
    const char* str, std::initializer_list<StrNFormatArg> args)
{
    return dpso::str::format(
        str,
        [&](const auto* name, std::size_t nameLen) -> const char*
        {
            for (const auto& arg : args)
                if (dpso::str::cmpSubStr(
                        arg.name, name, nameLen) == 0)
                    return arg.str.c_str();

            return {};
        });
}


}
