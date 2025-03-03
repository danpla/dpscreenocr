#include "json.h"

#include <cassert>

#include <jansson.h>

#include "dpso_utils/str.h"


namespace dpso::json {
namespace {


// json_type in Jansson has JSON_TRUE/FALSE instead of a proper
// JSON_BOOL, so use our own enum instead.
enum class JsonType {
    object,
    array,
    string,
    integer,
    real,
    boolean,
    null
};


const char* getName(JsonType type)
{
    #define CASE(N) case JsonType::N: return #N

    switch (type) {
    CASE(object);
    CASE(array);
    CASE(string);
    CASE(integer);
    CASE(real);
    CASE(boolean);
    CASE(null);
    }

    #undef CASE

    assert(false);
    return "";
}


JsonType getType(const json_t* json)
{
    assert(json);

    switch (json_typeof(json)) {
    case JSON_OBJECT:
        return JsonType::object;
    case JSON_ARRAY:
        return JsonType::array;
    case JSON_STRING:
        return JsonType::string;
    case JSON_INTEGER:
        return JsonType::integer;
    case JSON_REAL:
        return JsonType::real;
    case JSON_TRUE:
    case JSON_FALSE:
        return JsonType::boolean;
    case JSON_NULL:
        return JsonType::null;
    }

    assert(false);
    return {};
}


HandleUPtr loadJson(const char* data, JsonType type)
{
    assert(type == JsonType::array || type == JsonType::object);

    json_error_t error;
    HandleUPtr result{json_loads(data, 0, &error)};

    if (!result)
        throw Error{str::format(
            "{}:{}: {}", error.line, error.column, error.text)};

    if (getType(result.get()) != type)
        throw Error{str::format("Root is not {}", getName(type))};

    return result;
}


}


void HandleDeleter::operator()(Handle* h) const
{
    json_decref(h);
}


Object Object::load(const char* data)
{
    return Object{loadJson(data, JsonType::object)};
}


Object::Object(HandleUPtr h)
    : handle{std::move(h)}
{
    assert(handle);
    assert(getType(handle.get()) == JsonType::object);
}


static json_t* get(
    const json_t* object, const char* key, JsonType type)
{
    assert(object);
    assert(getType(object) == JsonType::object);
    assert(type != JsonType::null);

    auto* val = json_object_get(object, key);
    if (!val)
        throw Error{str::format("No \"{}\"", key)};

    if (getType(val) == JsonType::null)
        throw Error{str::format("\"{}\" is null", key)};

    if (getType(val) != type)
        throw Error{str::format(
            "\"{}\" is not {}", key, getName(type))};

    return val;
}


bool Object::getBool(const char* key) const
{
    return json_boolean_value(
        get(handle.get(), key, JsonType::boolean));
}


std::string Object::getStr(const char* key) const
{
    const auto* val = get(handle.get(), key, JsonType::string);
    return {json_string_value(val), json_string_length(val)};
}


std::int64_t Object::getInt(const char* key) const
{
    return json_integer_value(
        get(handle.get(), key, JsonType::integer));
}


Object Object::getObject(const char* key) const
{
    return Object{HandleUPtr{
        json_incref(get(handle.get(), key, JsonType::object))}};
}


Array Object::getArray(const char* key) const
{
    return Array{HandleUPtr{
        json_incref(get(handle.get(), key, JsonType::array))}};
}


Array Array::load(const char* data)
{
    return Array{loadJson(data, JsonType::array)};
}


Array::Array(HandleUPtr h)
    : handle{std::move(h)}
{
    assert(handle);
    assert(getType(handle.get()) == JsonType::array);
}


std::size_t Array::getSize() const
{
    return json_array_size(handle.get());
}


static json_t* get(
    const json_t* array, std::size_t idx, JsonType type)
{
    assert(array);
    assert(getType(array) == JsonType::array);
    assert(type != JsonType::null);

    const auto size = json_array_size(array);
    if (idx >= size)
        throw Error{str::format(
            "Index {} is out of bounds [0, {})", idx, size)};

    auto* val = json_array_get(array, idx);

    // We have already checked all the conditions under which
    // json_array_get() can return null according to the
    // documentation.
    assert(val);

    if (getType(val) == JsonType::null)
        throw Error{str::format("Value at index {} is null", idx)};

    if (getType(val) != type)
        throw Error{str::format(
            "Value at index {} is not {}", idx, getName(type))};

    return val;
}


bool Array::getBool(std::size_t idx) const
{
    return json_boolean_value(
        get(handle.get(), idx, JsonType::boolean));
}


std::string Array::getStr(std::size_t idx) const
{
    const auto* val = get(handle.get(), idx, JsonType::string);
    return {json_string_value(val), json_string_length(val)};
}


std::int64_t Array::getInt(std::size_t idx) const
{
    return json_integer_value(
        get(handle.get(), idx, JsonType::integer));
}


Object Array::getObject(std::size_t idx) const
{
    return Object{HandleUPtr{
        json_incref(get(handle.get(), idx, JsonType::object))}};
}


Array Array::getArray(std::size_t idx) const
{
    return Array{HandleUPtr{
        json_incref(get(handle.get(), idx, JsonType::array))}};
}


}
