
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


namespace {


struct KeyValue {
    std::string key;
    std::string value;
};


}


static std::vector<KeyValue> keyValues;


static int cmpKeys(const char* a, const char* b)
{
    return dpso::str::cmp(a, b, dpso::str::cmpIgnoreCase);
}


static decltype(keyValues)::iterator keyValuesLowerBound(
    const char* key)
{
    return std::lower_bound(
        keyValues.begin(), keyValues.end(), key,
        [](const KeyValue& kv, const char* key)
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
    const auto* end = valueBegin;
    for (; *end; ++end)
        if (!std::isspace(*end))
            valueEnd = end + 1;
}


static void assignUnescaped(
    std::string& str, const char* valueBegin, const char* valueEnd)
{
    str.clear();

    const auto* s = valueBegin;
    while (s < valueEnd) {
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


static void parseKeyValue(const char* str, KeyValue& kv)
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


static void dpsoCfgLoad(std::FILE* fp)
{
    keyValues.clear();

    KeyValue kv;
    for (std::string line; getLine(fp, line);) {
        parseKeyValue(line.c_str(), kv);
        if (!kv.key.empty())
            dpsoCfgSetStr(kv.key.c_str(), kv.value.c_str());
    }
}


int dpsoCfgLoad(const char* filePath)
{
    keyValues.clear();

    auto* fp = dpsoFopenUtf8(filePath, "r");
    if (!fp) {
        if (errno == ENOENT)
            return true;

        dpsoSetError((
            std::string{"dpsoFopenUtf8(..., \"r\") failed: "}
            + std::strerror(errno)).c_str());
        return false;
    }

    dpsoCfgLoad(fp);
    std::fclose(fp);

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
    std::FILE* fp, const KeyValue& kv, int maxKeyLen)
{
    std::fprintf(fp, "%-*s ", maxKeyLen, kv.key.c_str());

    const auto quote = needQuote(kv.value);
    if (quote)
        std::fputc('"', fp);

    const auto* s = kv.value.c_str();
    while (*s) {
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


static void dpsoCfgSave(std::FILE* fp)
{
    std::size_t maxKeyLen = 0;

    for (const auto& kv : keyValues)
        if (kv.key.size() > maxKeyLen)
            maxKeyLen = kv.key.size();

    for (const auto& kv : keyValues)
        writeKeyValue(fp, kv, maxKeyLen);
}


int dpsoCfgSave(const char* filePath)
{
    auto* fp = dpsoFopenUtf8(filePath, "w");
    if (!fp) {
        dpsoSetError((
            std::string{"dpsoFopenUtf8(..., \"w\") failed: "}
            + std::strerror(errno)).c_str());
        return false;
    }

    dpsoCfgSave(fp);
    std::fclose(fp);

    return true;
}


void dpsoCfgClear(void)
{
    keyValues.clear();
}


int dpsoCfgKeyExists(const char* key)
{
    return dpsoCfgGetStr(key, nullptr) != nullptr;
}


const char* dpsoCfgGetStr(const char* key, const char* defaultVal)
{
    const auto iter = keyValuesLowerBound(key);
    if (iter != keyValues.end()
            && cmpKeys(iter->key.c_str(), key) == 0)
        return iter->value.c_str();

    return defaultVal;
}


void dpsoCfgSetStr(const char* key, const char* val)
{
    const auto iter = keyValuesLowerBound(key);
    if (iter != keyValues.end() &&
            cmpKeys(iter->key.c_str(), key) == 0)
        iter->value = val;
    else
        keyValues.insert(iter, {key, val});
}


static const char* boolToStr(bool b)
{
    return b ? "true" : "false";
}


int dpsoCfgGetInt(const char* key, int defaultVal)
{
    const auto* str = dpsoCfgGetStr(key, "");
    if (!*str)
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


void dpsoCfgSetInt(const char* key, int val)
{
    dpsoCfgSetStr(key, std::to_string(val).c_str());
}


int dpsoCfgGetBool(const char* key, int defaultVal)
{
    return dpsoCfgGetInt(key, defaultVal) != 0;
}


void dpsoCfgSetBool(const char* key, int val)
{
    dpsoCfgSetStr(key, boolToStr(val));
}
