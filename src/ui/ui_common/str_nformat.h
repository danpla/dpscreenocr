#pragma once

#include <initializer_list>
#include <utility>

#include "dpso_utils/str.h"


namespace ui {


struct StrNFormatArg {
    StrNFormatArg(const char* name, const char* str)
        : name{name}
        , str{str}
    {
    }

    StrNFormatArg(const char* name, char* str)
        : name{name}
        , str{str}
    {
    }

    StrNFormatArg(const char* name, std::string str)
        : name{name}
        , str{std::move(str)}
    {
    }

    template<typename T>
    StrNFormatArg(const char* name, const T& v)
        : name{name}
    {
        using namespace dpso::str;
        str = toStr(v);
    }

    const char* name;
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
    const char* str, std::initializer_list<StrNFormatArg> args);


}
