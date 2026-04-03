#pragma once

#include <initializer_list>
#include <type_traits>

#include "dpso_utils/str.h"


namespace ui {


struct StrNFormatArg {
    StrNFormatArg(std::string_view name, std::string_view str)
        : name{name}
        , str{str}
    {
    }

    template<
        typename T,
        std::enable_if_t<
            !std::is_convertible_v<T, std::string_view>>* = {}>
    StrNFormatArg(std::string_view name, const T& v)
        : name{name}
    {
        using namespace dpso::str;
        str = toStr(v);
    }

    std::string_view name;
    std::string str;
};


// Python-style named string formatting.
//
// The function formats string similar to Python's str.format(),
// except it only supports named arguments. A partition in the str
// enclosed in braces defines a name to be replaced with the
// corresponding entry in the args array. To insert a brace as is,
// mention it twice.
std::string strNFormat(
    std::string_view str, std::initializer_list<StrNFormatArg> args);


}
