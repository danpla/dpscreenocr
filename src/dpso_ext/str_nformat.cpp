#include "str_nformat.h"

#include <cstddef>
#include <string>

#include "dpso_utils/str.h"
#include "dpso_utils/str_format_core.h"


const char* dpsoStrNFormat(
    const char* str, const DpsoStrNFormatArg* args, size_t numArgs)
{
    static std::string result;

    result = dpso::str::format(
        str,
        [&](const auto* name, std::size_t nameLen) -> const char*
        {
            for (std::size_t i = 0; i < numArgs; ++i)
                if (dpso::str::cmpSubStr(
                        args[i].name, name, nameLen) == 0)
                    return args[i].str;

            return {};
        });

    return result.c_str();
}
