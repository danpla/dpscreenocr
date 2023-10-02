
// This is not a JSON library per se, but rather a thin wrapper for
// Jansson. Its main purpose is to provide proper exception-based
// error handling. As a bonus, we also have stricter typing and
// automatic memory management.
//
// Similar to Jansson, Array and Object are reference-counted types;
// think of them as std::shared_ptr.

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>


struct json_t;  // From Jansson.


namespace dpso::json {


class Error : public std::runtime_error {
    using runtime_error::runtime_error;
};


using Handle = ::json_t;


struct HandleDeleter {
    void operator()(Handle* h) const;
};


using HandleUPtr = std::unique_ptr<Handle, HandleDeleter>;


class Array;


class Object {
public:
    static Object load(const char* data);

    bool getBool(const char* key) const;
    std::string getStr(const char* key) const;
    std::int64_t getInt(const char* key) const;
    Object getObject(const char* key) const;
    Array getArray(const char* key) const;
private:
    friend class Array;

    explicit Object(HandleUPtr h);

    HandleUPtr handle;
};


class Array {
public:
    static Array load(const char* data);

    std::size_t getSize() const;

    bool getBool(std::size_t idx) const;
    std::string getStr(std::size_t idx) const;
    std::int64_t getInt(std::size_t idx) const;
    Object getObject(std::size_t idx) const;
    Array getArray(std::size_t idx) const;
private:
    friend class Object;

    explicit Array(HandleUPtr h);

    HandleUPtr handle;
};


}
