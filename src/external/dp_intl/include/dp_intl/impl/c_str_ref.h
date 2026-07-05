#pragma once

#include <cstddef>


namespace dp::intl {


struct CStrRef {
    const char* data{""};
    std::size_t size{};

    CStrRef() = default;

    CStrRef(const char* data, std::size_t size)
        : data{data}
        , size{size}
    {
    }

    operator std::string_view() const
    {
        return {data, size};
    }

    operator const char*() const
    {
        return data;
    }
};


}
