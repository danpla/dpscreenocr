
#include "cfg.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "dpso/error.h"
#include "dpso/str.h"
#include "os.h"


// A config is a collection of key-value pairs in a text file. The
// format is designed to be simple and human-friendly: the first word
// of a line is a key, and the rest (with stripped whitespace) is a
// value. Empty lines are ignored.
//
// If a value in the file is enclosed in double quotes, the quotes
// will be removed. This allows to preserve leading and trailing
// whitespace. Obviously, quoting is also necessary if the value
// actually starts and ends with a double quote. In other cases
// (including a value consisting of a single double quote) quoting
// is optional.
//
// The characters \b, \f, \n, \r, and \t are escaped with a backslash.
// Any other character preceded by \ is inserted as is. Escaping \t is
// optional.


struct DpsoCfg {
    struct KeyValue {
        std::string key;
        std::string value;
    };

    std::vector<KeyValue> keyValues;
};


struct DpsoCfg* dpsoCfgCreate(void)
{
    return new DpsoCfg{};
}


void dpsoCfgDelete(struct DpsoCfg* cfg)
{
    delete cfg;
}


static int cmpKeys(const char* a, const char* b)
{
    return dpso::str::cmp(a, b, dpso::str::cmpIgnoreCase);
}


template <typename T>
static auto keyValuesLowerBound(
    T& keyValues, const char* key) -> decltype(keyValues.begin())
{
    return std::lower_bound(
        keyValues.begin(), keyValues.end(), key,
        [](const DpsoCfg::KeyValue& kv, const char* key)
        {
            return cmpKeys(kv.key.c_str(), key) < 0;
        });
}


static void getKeyValueBounds(
    const char* str,
    const char*& keyBegin, const char*& keyEnd,
    const char*& valueBegin, const char*& valueEnd)
{
    keyBegin = str;
    while (std::isspace(*keyBegin))
        ++keyBegin;

    keyEnd = keyBegin;
    while (*keyEnd && !std::isspace(*keyEnd))
        ++keyEnd;

    valueBegin = keyEnd;
    while (std::isspace(*valueBegin))
        ++valueBegin;

    valueEnd = valueBegin;
    for (const auto* s = valueBegin; *s; ++s)
        if (!std::isspace(*s))
            valueEnd = s + 1;
}


static void assignUnescaped(
    std::string& str, const char* valueBegin, const char* valueEnd)
{
    str.clear();

    for (const auto* s = valueBegin; s < valueEnd;) {
        const auto c = *s++;
        if (c != '\\') {
            str += c;
            continue;
        }

        if (s == valueEnd)
            break;

        const auto e = *s++;
        switch (e) {
            case 'b':
                str += '\b';
                break;
            case 'f':
                str += '\f';
                break;
            case 'n':
                str += '\n';
                break;
            case 'r':
                str += '\r';
                break;
            case 't':
                str += '\t';
                break;
            default:
                str += e;
                break;
        }
    }
}


static void parseKeyValue(const char* str, DpsoCfg::KeyValue& kv)
{
    const char* keyBegin;
    const char* keyEnd;
    const char* valueBegin;
    const char* valueEnd;
    getKeyValueBounds(str, keyBegin, keyEnd, valueBegin, valueEnd);

    if (valueEnd - valueBegin > 1
            && *valueBegin == '"'
            && valueEnd[-1] == '"') {
        ++valueBegin;
        --valueEnd;
    }

    kv.key.assign(keyBegin, keyEnd - keyBegin);
    assignUnescaped(kv.value, valueBegin, valueEnd);
}


static bool getLine(std::FILE* fp, std::string& line)
{
    line.clear();

    while (true) {
        const auto c = std::fgetc(fp);
        if (c == EOF)
            return !line.empty();
        if (c == '\n' || c == '\r')
            break;

        line += c;
    }

    return true;
}


int dpsoCfgLoad(struct DpsoCfg* cfg, const char* filePath)
{
    if (!cfg) {
        dpsoSetError("cfg is null");
        return false;
    }

    cfg->keyValues.clear();

    dpso::StdFileUPtr fp{dpsoFopenUtf8(filePath, "r")};
    if (!fp) {
        if (errno == ENOENT)
            return true;

        dpsoSetError(
            "dpsoFopenUtf8(..., \"r\") failed: %s",
            std::strerror(errno));
        return false;
    }

    DpsoCfg::KeyValue kv;
    for (std::string line; getLine(fp.get(), line);) {
        parseKeyValue(line.c_str(), kv);
        if (!kv.key.empty())
            dpsoCfgSetStr(cfg, kv.key.c_str(), kv.value.c_str());
    }

    return true;
}


static bool needQuote(const std::string& str)
{
    if (str.empty())
        return false;

    // \t will be escaped
    if (str.front() == ' ' || str.back() == ' ')
        return true;

    return str.size() > 1 && str.front() == '"' && str.back() == '"';
}


static void writeKeyValue(
    std::FILE* fp, const DpsoCfg::KeyValue& kv, int maxKeyLen)
{
    std::fprintf(fp, "%-*s ", maxKeyLen, kv.key.c_str());

    const auto quote = needQuote(kv.value);
    if (quote)
        std::fputc('"', fp);

    for (const auto* s = kv.value.c_str(); *s;) {
        const auto c = *s++;

        switch (c) {
            case '\b':
                std::fputs("\\b", fp);
                break;
            case '\f':
                std::fputs("\\f", fp);
                break;
            case '\n':
                std::fputs("\\n", fp);
                break;
            case '\r':
                std::fputs("\\r", fp);
                break;
            case '\t':
                std::fputs("\\t", fp);
                break;
            case '\\':
                std::fputs("\\\\", fp);
                break;
            default:
                std::fputc(c, fp);
                break;
        }
    }

    if (quote)
        std::fputc('"', fp);

    std::fputc('\n', fp);
}


int dpsoCfgSave(const struct DpsoCfg* cfg, const char* filePath)
{
    if (!cfg) {
        dpsoSetError("cfg is null");
        return false;
    }

    dpso::StdFileUPtr fp{dpsoFopenUtf8(filePath, "w")};
    if (!fp) {
        dpsoSetError(
            "dpsoFopenUtf8(..., \"w\") failed: %s",
            std::strerror(errno));
        return false;
    }

    std::size_t maxKeyLen = 0;

    for (const auto& kv : cfg->keyValues)
        if (kv.key.size() > maxKeyLen)
            maxKeyLen = kv.key.size();

    for (const auto& kv : cfg->keyValues)
        writeKeyValue(fp.get(), kv, maxKeyLen);

    return true;
}


void dpsoCfgClear(struct DpsoCfg* cfg)
{
    if (cfg)
        cfg->keyValues.clear();
}


int dpsoCfgKeyExists(const struct DpsoCfg* cfg, const char* key)
{
    return dpsoCfgGetStr(cfg, key, nullptr) != nullptr;
}


const char* dpsoCfgGetStr(
    const struct DpsoCfg* cfg,
    const char* key,
    const char* defaultVal)
{
    if (!cfg)
        return defaultVal;

    const auto iter = keyValuesLowerBound(cfg->keyValues, key);
    if (iter != cfg->keyValues.end()
            && cmpKeys(iter->key.c_str(), key) == 0)
        return iter->value.c_str();

    return defaultVal;
}


void dpsoCfgSetStr(
    struct DpsoCfg* cfg, const char* key, const char* val)
{
    if (!cfg)
        return;

    const auto iter = keyValuesLowerBound(cfg->keyValues, key);
    if (iter != cfg->keyValues.end() &&
            cmpKeys(iter->key.c_str(), key) == 0)
        iter->value = val;
    else
        cfg->keyValues.insert(iter, {key, val});
}


static const char* boolToStr(bool b)
{
    return b ? "true" : "false";
}


int dpsoCfgGetInt(
    const struct DpsoCfg* cfg, const char* key, int defaultVal)
{
    const auto* str = dpsoCfgGetStr(cfg, key, nullptr);
    if (!str)
        return defaultVal;

    char* end;
    const auto result = std::strtol(str, &end, 10);
    if (end != str) {
        // Don't treat string as an integer if it has any trailing
        // non-digit characters except whitespace. We intentionally
        // ignore floats (like "1.2"), as we currently don't have
        // Get/SetFloat() routines.
        //
        // Note that since we have at least one decimal digit in the
        // string, the test for boolean strings below is not needed.
        for (; *end; ++end)
            if (!std::isspace(*end))
                return defaultVal;

        return result;
    }

    for (int boolInt = 0; boolInt < 2; ++boolInt)
        if (dpso::str::cmp(
                str,
                boolToStr(boolInt),
                dpso::str::cmpIgnoreCase) == 0)
            return boolInt;

    return defaultVal;
}


void dpsoCfgSetInt(struct DpsoCfg* cfg, const char* key, int val)
{
    dpsoCfgSetStr(cfg, key, std::to_string(val).c_str());
}


int dpsoCfgGetBool(
    const struct DpsoCfg* cfg, const char* key, int defaultVal)
{
    return dpsoCfgGetInt(cfg, key, defaultVal) != 0;
}


void dpsoCfgSetBool(struct DpsoCfg* cfg, const char* key, int val)
{
    dpsoCfgSetStr(cfg, key, boolToStr(val));
}
