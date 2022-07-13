
#include "cfg.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "dpso/error.h"
#include "dpso/str.h"
#include "os.h"


struct DpsoCfg {
    struct KeyValue {
        std::string key;
        std::string value;
    };

    std::vector<KeyValue> keyValues;
};


DpsoCfg* dpsoCfgCreate(void)
{
    return new DpsoCfg{};
}


void dpsoCfgDelete(DpsoCfg* cfg)
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


// Note that we use isblank() rather than isspace() because we only
// care about spaces and tabs. With isspace(), we would also need to
// handle \f and \v especially, either by adding the corresponding
// escape sequences, or by wrapping a value in double quotes when it
// begins or ends with such a character.
static void parseKeyValue(const char* str, DpsoCfg::KeyValue& kv)
{
    const auto* keyBegin = str;
    while (std::isblank(*keyBegin))
        ++keyBegin;

    const auto* keyEnd = keyBegin;
    while (*keyEnd && !std::isblank(*keyEnd))
        ++keyEnd;

    kv.key.assign(keyBegin, keyEnd);

    const auto* valueBegin = keyEnd;
    while (std::isblank(*valueBegin))
        ++valueBegin;

    kv.value.clear();

    // We will trim any unescaped trailing blanks. \ at the end is
    // ignored, which allows to explicitly mark the end of the value
    // instead of escaping its last space.
    const auto* blanksBegin = valueBegin;
    const auto* blanksEnd = blanksBegin;

    for (const auto* s = valueBegin; *s;) {
        if (std::isblank(*s)) {
            if (s != blanksEnd)
                blanksBegin = s;

            blanksEnd = ++s;
            continue;
        }

        if (s == blanksEnd)
            kv.value.append(blanksBegin, blanksEnd);

        const auto c = *s++;
        if (c != '\\') {
            kv.value += c;
            continue;
        }

        if (!*s)
            break;

        const auto e = *s++;
        switch (e) {
            case 'n':
                kv.value += '\n';
                break;
            case 'r':
                kv.value += '\r';
                break;
            case 't':
                kv.value += '\t';
                break;
            default:
                kv.value += e;
                break;
        }
    }
}


static bool getLine(std::FILE* fp, std::string& line)
{
    line.clear();

    while (true) {
        const auto c = std::fgetc(fp);
        if (c == EOF)
            return !line.empty();

        // We don't care about line ending formats (e.g. CRLF) since
        // empty lines are ignored anyway.
        if (c == '\n' || c == '\r')
            break;

        line += c;
    }

    return true;
}


// Our file format allows using any byte values, so we don't use the
// text mode (i.e. fopen() without the "b" flag) for IO - neither for
// reading nor for writing - to avoid surprises when we actually need
// to handle "binary" data (for example, the text mode on Windows
// treats 0x1a as EOF when reading). We will still write CRLF line
// endings on Windows to make Notepad users happy.


int dpsoCfgLoad(DpsoCfg* cfg, const char* filePath)
{
    if (!cfg) {
        dpsoSetError("cfg is null");
        return false;
    }

    cfg->keyValues.clear();

    dpso::StdFileUPtr fp{dpsoFopenUtf8(filePath, "rb")};
    if (!fp) {
        if (errno == ENOENT)
            return true;

        dpsoSetError(
            "dpsoFopenUtf8(..., \"rb\") failed: %s",
            std::strerror(errno));
        return false;
    }

    std::string line;
    DpsoCfg::KeyValue kv;
    while (getLine(fp.get(), line)) {
        parseKeyValue(line.c_str(), kv);
        if (!kv.key.empty())
            dpsoCfgSetStr(cfg, kv.key.c_str(), kv.value.c_str());
    }

    return true;
}


static void writeKeyValue(
    std::FILE* fp, const DpsoCfg::KeyValue& kv, int maxKeyLen)
{
    std::fprintf(fp, "%-*s ", maxKeyLen, kv.key.c_str());

    if (!kv.value.empty() && kv.value.front() == ' ')
        std::fputc('\\', fp);

    for (const auto* s = kv.value.c_str(); *s;) {
        const auto c = *s++;

        switch (c) {
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

    // If we have a single space, it's already escaped, but we still
    // append \ to make the end visible.
    if (!kv.value.empty() && kv.value.back() == ' ')
        std::fputc('\\', fp);

    // Use CRLF on Windows to make Notepad users happy.
    #ifdef _WIN32
    std::fputc('\r', fp);
    #endif

    std::fputc('\n', fp);
}


int dpsoCfgSave(const DpsoCfg* cfg, const char* filePath)
{
    if (!cfg) {
        dpsoSetError("cfg is null");
        return false;
    }

    dpso::StdFileUPtr fp{dpsoFopenUtf8(filePath, "wb")};
    if (!fp) {
        dpsoSetError(
            "dpsoFopenUtf8(..., \"wb\") failed: %s",
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


void dpsoCfgClear(DpsoCfg* cfg)
{
    if (cfg)
        cfg->keyValues.clear();
}


int dpsoCfgKeyExists(const DpsoCfg* cfg, const char* key)
{
    return dpsoCfgGetStr(cfg, key, nullptr) != nullptr;
}


const char* dpsoCfgGetStr(
    const DpsoCfg* cfg, const char* key, const char* defaultVal)
{
    if (!cfg)
        return defaultVal;

    const auto iter = keyValuesLowerBound(cfg->keyValues, key);
    if (iter != cfg->keyValues.end()
            && cmpKeys(iter->key.c_str(), key) == 0)
        return iter->value.c_str();

    return defaultVal;
}


static bool isValidKey(const char* key)
{
    if (!*key)
        return false;

    for (const auto* s = key; *s; ++s)
        if (std::isblank(*s) || *s == '\r' || *s == '\n')
            return false;

    return true;
}


void dpsoCfgSetStr(DpsoCfg* cfg, const char* key, const char* val)
{
    if (!cfg)
        return;

    if (!isValidKey(key))
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


int dpsoCfgGetInt(const DpsoCfg* cfg, const char* key, int defaultVal)
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

        if (result >= INT_MAX)
            return INT_MAX;

        if (result <= INT_MIN)
            return INT_MIN;

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


void dpsoCfgSetInt(DpsoCfg* cfg, const char* key, int val)
{
    dpsoCfgSetStr(cfg, key, std::to_string(val).c_str());
}


int dpsoCfgGetBool(
    const DpsoCfg* cfg, const char* key, int defaultVal)
{
    return dpsoCfgGetInt(cfg, key, defaultVal) != 0;
}


void dpsoCfgSetBool(DpsoCfg* cfg, const char* key, int val)
{
    dpsoCfgSetStr(cfg, key, boolToStr(val));
}
