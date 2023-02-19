
#include "cfg.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "dpso_utils/error.h"
#include "dpso_utils/os.h"
#include "dpso_utils/str.h"


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


template<typename T>
static auto keyValuesLowerBound(T& keyValues, const char* key)
{
    return std::lower_bound(
        keyValues.begin(), keyValues.end(), key,
        [](const DpsoCfg::KeyValue& kv, const char* key)
        {
            return cmpKeys(kv.key.c_str(), key) < 0;
        });
}


static void parseKeyValue(const char* str, DpsoCfg::KeyValue& kv)
{
    const auto* keyBegin = str;
    while (dpso::str::isBlank(*keyBegin))
        ++keyBegin;

    const auto* keyEnd = keyBegin;
    while (*keyEnd && !dpso::str::isBlank(*keyEnd))
        ++keyEnd;

    kv.key.assign(keyBegin, keyEnd - keyBegin);

    const auto* valueBegin = keyEnd;
    while (dpso::str::isBlank(*valueBegin))
        ++valueBegin;

    kv.value.clear();

    // We will trim any unescaped trailing blanks. \ at the end is
    // ignored, which allows to explicitly mark the end of the value
    // instead of escaping its last space.
    const auto* blanksBegin = valueBegin;
    const auto* blanksEnd = blanksBegin;

    for (const auto* s = valueBegin; *s;) {
        if (dpso::str::isBlank(*s)) {
            if (s != blanksEnd)
                blanksBegin = s;

            blanksEnd = ++s;
            continue;
        }

        if (s == blanksEnd)
            kv.value.append(blanksBegin, blanksEnd - blanksBegin);

        if (const auto c = *s++; c != '\\') {
            kv.value += c;
            continue;
        }

        if (!*s)
            break;

        switch (const auto c = *s++) {
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
            kv.value += c;
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


bool dpsoCfgLoad(DpsoCfg* cfg, const char* filePath)
{
    if (!cfg) {
        dpsoSetError("cfg is null");
        return false;
    }

    cfg->keyValues.clear();

    dpso::StdFileUPtr fp{dpsoFopen(filePath, "rb")};
    if (!fp) {
        if (errno == ENOENT)
            return true;

        dpsoSetError(
            "dpsoFopen(..., \"rb\"): %s", std::strerror(errno));
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

    for (const auto* s = kv.value.c_str(); *s; ++s) {
        switch (const auto c = *s) {
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


bool dpsoCfgSave(const DpsoCfg* cfg, const char* filePath)
{
    if (!cfg) {
        dpsoSetError("cfg is null");
        return false;
    }

    dpso::StdFileUPtr fp{dpsoFopen(filePath, "wb")};
    if (!fp) {
        dpsoSetError(
            "dpsoFopen(..., \"wb\"): %s", std::strerror(errno));
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


bool dpsoCfgKeyExists(const DpsoCfg* cfg, const char* key)
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
    return *key && !std::strpbrk(key, " \t\r\n");
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


int dpsoCfgGetInt(const DpsoCfg* cfg, const char* key, int defaultVal)
{
    const auto* str = dpsoCfgGetStr(cfg, key, nullptr);
    if (!str)
        return defaultVal;

    // Skip whitespace manually since strtol() uses locale-dependent
    // isspace() under the hood.
    while (dpso::str::isSpace(*str))
        ++str;

    char* end;
    const auto result = std::strtol(str, &end, 10);
    if (end == str)
        return defaultVal;

    // Don't treat string as an integer if it has any trailing
    // non-digit characters except whitespace.
    for (; *end; ++end)
        if (!dpso::str::isSpace(*end))
            return defaultVal;

    return std::clamp<long>(result, INT_MIN, INT_MAX);
}


void dpsoCfgSetInt(DpsoCfg* cfg, const char* key, int val)
{
    dpsoCfgSetStr(cfg, key, std::to_string(val).c_str());
}


static const char* boolToStr(bool b)
{
    return b ? "true" : "false";
}


bool dpsoCfgGetBool(
    const DpsoCfg* cfg, const char* key, bool defaultVal)
{
    const auto* str = dpsoCfgGetStr(cfg, key, nullptr);
    if (!str)
        return defaultVal;

    for (int i = 0; i < 2; ++i)
        if (dpso::str::cmp(
                str,
                boolToStr(i),
                dpso::str::cmpIgnoreCase) == 0)
            return i;

    return defaultVal;
}


void dpsoCfgSetBool(DpsoCfg* cfg, const char* key, bool val)
{
    dpsoCfgSetStr(cfg, key, boolToStr(val));
}
