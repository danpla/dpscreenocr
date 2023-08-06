
#include "history.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "dpso_utils/error.h"
#include "dpso_utils/os.h"


// Each entry in a history file consists of a timestamp, two line
// feeds (\n\n), and the actual text. Entries are separated by a form
// feed and a line feed (\f\n).


struct DpsoHistory {
    struct Entry {
        std::string timestamp;
        std::string text;
    };

    std::string filePath;
    std::vector<Entry> entries;
    dpso::os::StdFileUPtr fp;
};


static bool loadData(const char* filePath, std::string& data)
{
    data.clear();

    dpso::os::StdFileUPtr fp{dpso::os::fopen(filePath, "rb")};
    if (!fp) {
        if (errno == ENOENT)
            return true;

        dpsoSetError(
            "os::fopen(..., \"rb\"): %s", std::strerror(errno));
        return false;
    }

    if (std::fseek(fp.get(), 0, SEEK_END) != 0) {
        dpsoSetError(
            "fseek(..., SEEK_END): %s", std::strerror(errno));
        return false;
    }

    const auto size = std::ftell(fp.get());
    if (size < 0) {
        dpsoSetError("ftell(): %s", std::strerror(errno));
        return false;
    }

    if (std::fseek(fp.get(), 0, SEEK_SET) != 0) {
        dpsoSetError(
            "fseek(..., 0, SEEK_SET): %s", std::strerror(errno));
        return false;
    }

    data.resize(size);
    if (std::fread(data.data(), 1, size, fp.get())
            != static_cast<std::size_t>(size)) {
        dpsoSetError("fread() failed");
        return false;
    }

    return true;
}


static bool createEntries(
    const char* data, std::vector<DpsoHistory::Entry>& entries)
{
    entries.clear();

    for (const auto* s = data; *s;) {
        const auto* timestampBegin = s;
        while (*s && *s != '\n')
            ++s;

        if (*s != '\n' || s[1] != '\n') {
            dpsoSetError(
                "No \\n\\n terminator for timestamp at %zu\n",
                timestampBegin - data);
            return false;
        }

        DpsoHistory::Entry e;
        e.timestamp.assign(timestampBegin, s - timestampBegin);

        s += 2;

        const auto* textBegin = s;
        while (*s && *s != '\f')
            ++s;

        e.text.assign(textBegin, s - textBegin);

        entries.push_back(std::move(e));

        if (*s == '\f') {
            if (s[1] != '\n') {
                dpsoSetError(
                    "Unexpected 0x%hhx after \\f at %zu",
                    s[1], s - data);
                return false;
            }

            if (!s[2]) {
                dpsoSetError("\\f\\n at end of file");
                return false;
            }

            s += 2;
        }
    }

    return true;
}


static dpso::os::StdFileUPtr openSync(
    const char* filePath, const char* mode)
{
    dpso::os::StdFileUPtr fp{dpso::os::fopen(filePath, mode)};
    if (!fp) {
        dpsoSetError(
            "os::fopen(..., \"%s\"): %s", mode, std::strerror(errno));
        return nullptr;
    }

    try {
        dpso::os::syncFileDir(filePath);
    } catch (dpso::os::Error& e) {
        dpsoSetError("os::syncFileDir(): %s", e.what());
        return nullptr;
    }

    return fp;
}


DpsoHistory* dpsoHistoryOpen(const char* filePath)
{
    auto history = std::make_unique<DpsoHistory>();
    history->filePath = filePath;

    std::string data;
    if (!loadData(filePath, data)
            || !createEntries(data.c_str(), history->entries))
        return nullptr;

    history->fp = openSync(filePath, "ab");
    if (!history->fp)
        return nullptr;

    return history.release();
}


void dpsoHistoryClose(DpsoHistory* history)
{
    delete history;
}


int dpsoHistoryCount(const DpsoHistory* history)
{
    return history ? history->entries.size() : 0;
}


static std::string replaceReservedChar(
    const char* text, char reservedChar)
{
    std::string result(std::strlen(text), 0);

    for (std::size_t i = 0; i < result.size(); ++i) {
        const auto c = text[i];
        result[i] = c == reservedChar ? ' ' : c;
    }

    return result;
}


const auto* const failureStateErrorMsg =
    "History is in failure state and is read-only";


bool dpsoHistoryAppend(
    DpsoHistory* history, const DpsoHistoryEntry* entry)
{
    if (!history) {
        dpsoSetError("history is null");
        return false;
    }

    if (!entry) {
        dpsoSetError("entry is null");
        return false;
    }

    if (!history->fp) {
        dpsoSetError(failureStateErrorMsg);
        return false;
    }

    DpsoHistory::Entry e{
        replaceReservedChar(entry->timestamp, '\n'),
        replaceReservedChar(entry->text, '\f')
    };

    auto* fp = history->fp.get();
    if ((!history->entries.empty() && std::fputs("\f\n", fp) == EOF)
            || std::fputs(e.timestamp.c_str(), fp) == EOF
            || std::fputs("\n\n", fp) == EOF
            || std::fputs(e.text.c_str(), fp) == EOF) {
        dpsoSetError("fputs() failed");
        history->fp.reset();
        return false;
    }

    if (std::fflush(fp) == EOF) {
        dpsoSetError("fflush() failed");
        history->fp.reset();
        return false;
    }

    try {
        dpso::os::syncFile(fp);
    } catch (dpso::os::Error& e) {
        dpsoSetError("os::syncFile(): %s", e.what());
        history->fp.reset();
        return false;
    }

    history->entries.push_back(std::move(e));

    return true;
}


void dpsoHistoryGet(
    const DpsoHistory* history, int idx, DpsoHistoryEntry* entry)
{
    if (!entry)
        return;

    if (!history
            || idx < 0
            || static_cast<std::size_t>(idx)
                >= history->entries.size()) {
        entry->timestamp = "";
        entry->text = "";
        return;
    }

    const auto& e = history->entries[idx];
    entry->timestamp = e.timestamp.c_str();
    entry->text = e.text.c_str();
}


bool dpsoHistoryClear(DpsoHistory* history)
{
    if (!history) {
        dpsoSetError("history is null");
        return false;
    }

    if (!history->fp) {
        dpsoSetError(failureStateErrorMsg);
        return false;
    }

    history->entries.clear();
    history->fp.reset();

    history->fp = openSync(history->filePath.c_str(), "wb");
    return history->fp != nullptr;
}
