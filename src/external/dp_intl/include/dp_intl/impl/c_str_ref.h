#pragma once

#include <cstddef>


namespace dp::intl {


// CStrRef is an adapter of strings returned by MessageCatalog (C
// strings of known length) to the types used by the Translator class
// overloads (plain C strings and std::string_view). Its purpose is to
// simplify generic code by using type conversion operators and to
// optimize the constriction of std::string_view (returning a C string
// from MessageCatalog would also work, but would require
// std::string_view to calculate the length at runtime).
class CStrRef {
public:
    CStrRef() = default;

    CStrRef(const char* data, std::size_t size)
        : data{data}
        , size{size}
    {
    }

    bool empty() const
    {
        return size == 0;
    }

    operator std::string_view() const
    {
        return {data, size};
    }

    operator const char*() const
    {
        return data;
    }
private:
    const char* data{""};
    std::size_t size{};
};


}
