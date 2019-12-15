
#include "cfg_private.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "cfg_path.h"
#include "dpso/str.h"


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
// The characters \b, \f, \n, \r, \t, and \ are escaped with a
// backslash. Escaping \t is optional. Escaping \ is optional too,
// unless the \ and the next character create one of the mentioned
// escape sequences.


namespace {


struct KeyValue {
    std::string key;
    std::string value;
};


}


static std::vector<KeyValue> keyValues;


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
    while (*end) {
        if (!std::isspace(*end))
            valueEnd = end + 1;

        ++end;
    }
}


static void assignUnescaped(
    std::string& str, const char* valueBegin, const char* valueEnd)
{
    const auto* s = valueBegin;
    while (s < valueEnd) {
        const auto c = *s;
        ++s;

        if (c == '\\' && *s) {
            const auto e = *s;
            ++s;

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
                case '\\':
                    str += '\\';
                    break;
                default:
                    str += c;
                    str += e;
                    break;
            }
        } else
            str += c;
    }
}


static KeyValue parseKeyValue(const char* str)
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

    KeyValue kv;
    kv.key.assign(keyBegin, keyEnd - keyBegin);
    assignUnescaped(kv.value, valueBegin, valueEnd);

    return kv;
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


static bool cmpByKeyIc(const KeyValue& kv, const char* key)
{
    return dpso::str::cmpIc(kv.key.c_str(), key) < 0;
}


static decltype(keyValues)::iterator keyValuesLowerBound(
    const char* key, bool* found = nullptr)
{
    auto iter = std::lower_bound(
        keyValues.begin(), keyValues.end(), key, cmpByKeyIc);

    if (found)
        *found = (
            iter != keyValues.end()
            && dpso::str::cmpIc(iter->key.c_str(), key) == 0);

    return iter;
}


void dpsoCfgLoadFp(FILE* fp)
{
    keyValues.clear();

    for (std::string line; getLine(fp, line);) {
        auto kv = parseKeyValue(line.c_str());
        if (kv.key.empty())
            continue;

        bool found;
        auto iter = keyValuesLowerBound(kv.key.c_str(), &found);
        if (found)
            iter->value.swap(kv.value);
        else
            keyValues.insert(iter, std::move(kv));
    }
}


void dpsoCfgLoad(const char* appName, const char* cfgName)
{
    keyValues.clear();

    auto* fp = dpsoCfgPathFopen(appName, cfgName, "r");
    if (!fp)
        return;

    dpsoCfgLoadFp(fp);
    std::fclose(fp);
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
    std::FILE* fp, int maxKeyLen, const KeyValue& kv)
{
    std::fprintf(fp, "%-*s ", maxKeyLen, kv.key.c_str());

    const auto quote = needQuote(kv.value);
    if (quote)
        std::fputc('"', fp);

    const auto* s = kv.value.c_str();
    while (*s) {
        const auto c = *s;
        ++s;

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


void dpsoCfgSaveFp(FILE* fp)
{
    int maxKeyLen = 0;

    for (const auto& kv : keyValues)
        if (static_cast<int>(kv.key.size()) > maxKeyLen)
            maxKeyLen = kv.key.size();

    for (const auto& kv : keyValues)
        writeKeyValue(fp, maxKeyLen, kv);
}


void dpsoCfgSave(const char* appName, const char* cfgName)
{
    auto* fp = dpsoCfgPathFopen(appName, cfgName, "w");
    if (!fp)
        return;

    dpsoCfgSaveFp(fp);
    std::fclose(fp);
}


void dpsoCfgClear(void)
{
    keyValues.clear();
}


int dpsoCfgKeyExists(const char* key)
{
    bool found;
    keyValuesLowerBound(key, &found);
    return found;
}


const char* dpsoCfgGetStr(const char* key, const char* defaultVal)
{
    bool found;
    const auto iter = keyValuesLowerBound(key, &found);
    if (found)
        return iter->value.c_str();

    return defaultVal;
}


void dpsoCfgSetStr(const char* key, const char* val)
{
    bool found;
    auto iter = keyValuesLowerBound(key, &found);
    if (found)
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
        if (dpso::str::cmpIc(str, boolToStr(boolInt)) == 0)
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
