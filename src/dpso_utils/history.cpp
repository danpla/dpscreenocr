
#include "history.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "dpso/error.h"
#include "os.h"


// Each entry in a history file consists of a timestamp, two line
// feeds (\n\n), and the actual text. Entries are separated by a form
// feed and a line feed (\f\n).


namespace {


struct Entry {
    std::size_t timestampPos;
    std::size_t textPos;
};


}


static std::string data;
static std::vector<Entry> entries;


static void loadData(std::FILE* fp)
{
    std::fseek(fp, 0, SEEK_END);
    const auto size = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);

    if (size < 0)
        return;

    data.resize(size);
    if (std::fread(&data[0], 1, size, fp)
            != static_cast<std::size_t>(size))
        data.clear();
}


static void createEntries()
{
    auto* s = &data[0];
    const auto* sBegin = s;
    const auto* validDataEnd = sBegin;

    while (*s) {
        Entry entry;

        entry.timestampPos = s - sBegin;

        while (*s && *s != '\n')
            ++s;

        if (*s != '\n' || s[1] != '\n')
            break;

        *s = 0;
        s += 2;

        entry.textPos = s - sBegin;
        while (*s && *s != '\f')
            ++s;

        entries.push_back(entry);

        validDataEnd = s;

        if (*s == '\f') {
            *s = 0;

            if (s[1] != '\n')
                break;

            s += 2;
        }
    }

    // Trim unused data in case of errors
    data.resize(validDataEnd - sBegin);

    if (!data.empty())
        data += '\0';
}


static void dpsoHistoryLoad(FILE* fp)
{
    dpsoHistoryClear();

    loadData(fp);
    createEntries();
}


int dpsoHistoryLoad(const char* filePath)
{
    dpsoHistoryClear();

    auto* fp = dpsoFopenUtf8(filePath, "rb");
    if (!fp) {
        if (errno == ENOENT)
            return true;

        dpsoSetError((
            std::string{"dpsoFopenUtf8(..., \"rb\") failed: "}
            + std::strerror(errno)).c_str());
        return false;
    }

    dpsoHistoryLoad(fp);
    std::fclose(fp);

    return true;
}


static void dpsoHistorySave(FILE* fp)
{
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (i > 0)
            std::fputs("\f\n", fp);

        const auto& e = entries[i];
        std::fputs(data.data() + e.timestampPos, fp);
        std::fputs("\n\n", fp);
        std::fputs(data.data() + e.textPos, fp);
    }
}


int dpsoHistorySave(const char* filePath)
{
    auto* fp = dpsoFopenUtf8(filePath, "wb");
    if (!fp) {
        dpsoSetError((
            std::string{"dpsoFopenUtf8(..., \"wb\") failed: "}
            + std::strerror(errno)).c_str());
        return false;
    }

    dpsoHistorySave(fp);
    std::fclose(fp);

    return true;
}


int dpsoHistoryCount(void)
{
    return entries.size();
}


void dpsoHistoryAppend(const struct DpsoHistoryEntry* entry)
{
    if (!entry)
        return;

    assert(data.empty() || data.back() == '\0');

    Entry e;

    e.timestampPos = data.size();
    data += entry->timestamp;
    data += '\0';

    e.textPos = data.size();
    data += entry->text;
    data += '\0';

    entries.push_back(e);
}


void dpsoHistoryGet(int idx, struct DpsoHistoryEntry* entry)
{
    if (!entry)
        return;

    if (idx < 0 || static_cast<std::size_t>(idx) >= entries.size()) {
        entry->text = "";
        entry->timestamp = "";
        return;
    }

    const auto& e = entries[idx];
    entry->text = data.data() + e.textPos;
    entry->timestamp = data.data() + e.timestampPos;
}


void dpsoHistoryClear(void)
{
    data.clear();
    entries.clear();
}
