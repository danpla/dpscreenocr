#include "cfg.h"

#include <algorithm>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "dpso_utils/error_set.h"
#include "dpso_utils/line_reader.h"
#include "dpso_utils/os.h"
#include "dpso_utils/str.h"
#include "dpso_utils/stream/file_stream.h"
#include "dpso_utils/stream/utils.h"


using namespace dpso;


namespace {


struct KeyValue {
    std::string key;
    std::string value;
};


using KeyValues = std::vector<KeyValue>;


template<typename T>
auto getLowerBound(T& keyValues, std::string_view key)
{
    return std::lower_bound(
        keyValues.begin(), keyValues.end(), key,
        [](const KeyValue& kv, std::string_view key)
        {
            return kv.key < key;
        });
}


const std::string* getStr(
    const KeyValues& keyValues, std::string_view key)
{
    const auto iter = getLowerBound(keyValues, key);
    if (iter != keyValues.end() && iter->key == key)
        return &iter->value;

    return {};
}


bool isValidKey(std::string_view key)
{
    return !key.empty() && key.find_first_of(" \t\r\n") == key.npos;
}


void setStr(
    KeyValues& keyValues, std::string_view key, std::string&& val)
{
    if (!isValidKey(key))
        return;

    auto iter = getLowerBound(keyValues, key);
    if (iter == keyValues.end() || iter->key != key)
        iter = keyValues.insert(iter, {std::string{key}, {}});

    iter->value = std::move(val);
}


void loadKeyValue(KeyValues& keyValues, std::string_view str)
{
    str = str::trimLeft(str, str::isBlank);

    const auto key = str.substr(
        0,
        std::find_if(str.begin(), str.end(), str::isBlank)
            - str.begin());

    const auto rawVal = str::trim(
        str.substr(key.size()), str::isBlank);

    std::string val;
    for (auto iter = rawVal.begin(); iter < rawVal.end();) {
        const auto unsecapedEnd = std::find(iter, rawVal.end(), '\\');
        val.append(iter, unsecapedEnd);

        if (unsecapedEnd == rawVal.end())
            break;

        iter = unsecapedEnd + 1;
        if (iter == rawVal.end())
            break;

        switch (const auto c = *iter++) {
        case 'n':
            val += '\n';
            break;
        case 'r':
            val += '\r';
            break;
        case 't':
            val += '\t';
            break;
        default:
            val += c;
            break;
        }
    }

    setStr(keyValues, key, std::move(val));
}


void writeKeyValue(
    Stream& stream, const KeyValue& kv, std::size_t maxKeyLen)
{
    write(stream, str::justifyLeft(kv.key, maxKeyLen + 1));

    if (!kv.value.empty() && kv.value.front() == ' ')
        write(stream, '\\');

    for (auto c : kv.value)
        switch (c) {
        case '\n':
            write(stream, "\\n");
            break;
        case '\r':
            write(stream, "\\r");
            break;
        case '\t':
            write(stream, "\\t");
            break;
        case '\\':
            write(stream, "\\\\");
            break;
        default:
            write(stream, c);
            break;
        }

    if (!kv.value.empty() && kv.value.back() == ' ')
        write(stream, '\\');

    write(stream, os::newline);
}


}


struct DpsoCfg {
    KeyValues keyValues;
};


DpsoCfg* dpsoCfgCreate(void)
{
    return new DpsoCfg{};
}


void dpsoCfgDelete(DpsoCfg* cfg)
{
    delete cfg;
}


bool dpsoCfgLoad(DpsoCfg* cfg, const char* filePath)
{
    if (!cfg) {
        setError("cfg is null");
        return false;
    }

    cfg->keyValues.clear();

    std::optional<FileStream> file;
    try {
        file.emplace(filePath, FileStream::Mode::read);
    } catch (os::FileNotFoundError&) {
        return true;
    } catch (os::Error& e) {
        setError("FileStream(..., Mode::read): {}", e.what());
        return false;
    }

    LineReader lineReader{*file};
    for (std::string line; true;) {
        try {
            if (!lineReader.readLine(line))
                break;
        } catch (StreamError& e) {
            setError("LineReader::readLine(): {}", e.what());
            cfg->keyValues.clear();
            return false;
        }

        loadKeyValue(cfg->keyValues, line);
    }

    return true;
}


bool dpsoCfgSave(const DpsoCfg* cfg, const char* filePath)
{
    if (!cfg) {
        setError("cfg is null");
        return false;
    }

    if (const auto fileDir = os::getDirName(filePath);
            !fileDir.empty())
        try {
            os::makeDirs(fileDir);
        } catch (os::Error& e) {
            setError("os::makeDirs(): {}", e.what());
            return false;
        }

    std::optional<FileStream> file;
    try {
        file.emplace(filePath, FileStream::Mode::write);
    } catch (os::Error& e) {
        setError("FileStream(..., Mode::write): {}", e.what());
        return false;
    }

    std::size_t maxKeyLen{};
    for (const auto& kv : cfg->keyValues)
        maxKeyLen = std::max(maxKeyLen, kv.key.size());

    try {
        for (const auto& kv : cfg->keyValues)
            writeKeyValue(*file, kv, maxKeyLen);
    } catch (StreamError& e) {
        setError("{}", e.what());
        return false;
    }

    return true;
}


void dpsoCfgClear(DpsoCfg* cfg)
{
    if (cfg)
        cfg->keyValues.clear();
}


bool dpsoCfgKeyExists(const DpsoCfg* cfg, const char* key)
{
    return dpsoCfgGetStr(cfg, key, nullptr) != nullptr;
}


const char* dpsoCfgGetStr(
    const DpsoCfg* cfg, const char* key, const char* defaultVal)
{
    if (!cfg)
        return defaultVal;

    const auto* str = getStr(cfg->keyValues, key);
    return str ? str->c_str() : defaultVal;
}


void dpsoCfgSetStr(DpsoCfg* cfg, const char* key, const char* val)
{
    if (cfg)
        setStr(cfg->keyValues, key, val);
}


int dpsoCfgGetInt(const DpsoCfg* cfg, const char* key, int defaultVal)
{
    if (!cfg)
        return defaultVal;

    const auto* str = getStr(cfg->keyValues, key);
    if (!str)
        return defaultVal;

    // There's no sense to allow leading or trailing whitespace around
    // numbers, since it can only occur when explicitly included,
    // implying that the user probably really wanted a string.

    int result{};

    const auto* strEnd = str->data() + str->size();
    const auto [ptr, ec] = std::from_chars(
        str->data(), strEnd, result);
    if (ec == std::errc{} && ptr == strEnd)
        return result;

    return defaultVal;
}


void dpsoCfgSetInt(DpsoCfg* cfg, const char* key, int val)
{
    if (cfg)
        setStr(cfg->keyValues, key, str::toStr(val));
}


static std::string_view boolToStr(bool b)
{
    return b ? "true" : "false";
}


bool dpsoCfgGetBool(
    const DpsoCfg* cfg, const char* key, bool defaultVal)
{
    if (!cfg)
        return defaultVal;

    const auto* str = getStr(cfg->keyValues, key);
    if (!str)
        return defaultVal;

    for (int i = 0; i < 2; ++i)
        if (str::equalIgnoreCase(*str, boolToStr(i)))
            return i;

    return defaultVal;
}


void dpsoCfgSetBool(DpsoCfg* cfg, const char* key, bool val)
{
    if (cfg)
        setStr(cfg->keyValues, key, std::string{boolToStr(val)});
}
