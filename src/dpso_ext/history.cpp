
#include "history.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"


using namespace dpso;


// Each entry in a history file consists of a timestamp, two line
// feeds (\n\n), and the actual text. Entries are separated by a form
// feed and a line feed (\f\n).


struct DpsoHistory {
    struct Entry {
        std::string timestamp;
        std::string text;
    };

    std::string filePath;
    os::StdFileUPtr fp;
    std::vector<Entry> entries;
};


static bool loadData(const char* filePath, std::string& data)
{
    data.clear();

    try {
        data.resize(os::getFileSize(filePath));
    } catch (os::FileNotFoundError&) {
        return true;
    } catch (os::Error& e) {
        setError("os::getFileSize(): {}", e.what());
        return false;
    }

    os::StdFileUPtr fp{os::fopen(filePath, "rb")};
    if (!fp) {
        setError(
            "os::fopen(..., \"rb\"): {}", os::getErrnoMsg(errno));
        return false;
    }

    try {
        os::read(fp.get(), data.data(), data.size());
    } catch (os::Error& e) {
        setError("os::read(): {}", e.what());
        return false;
    }

    return true;
}


static bool createEntries(
    const char* data,
    std::vector<DpsoHistory::Entry>& entries,
    std::size_t& validDataSize)
{
    entries.clear();
    validDataSize = 0;

    // Note that we don't treat truncation as an error. Since the
    // history is append-only, we assume that the data is most likely
    // truncated due to a partial write (e.g., lack of disk space or
    // power loss), so the best strategy is to restore successfully
    // written entries.

    for (const auto* s = data; *s;) {
        const auto* timestampBegin = s;
        while (*s && *s != '\n')
            ++s;

        if (!*s)
            // Truncated timestamp.
            break;

        if (!s[1])
            // Truncated \n\n terminator.
            break;

        if (s[1] != '\n') {
            setError(
                "Unexpected {:#04x} instead of \\n at {} for "
                "timestamp at {}",
                s[1],  s - data, timestampBegin - data);
            return false;
        }

        DpsoHistory::Entry e;
        e.timestamp.assign(timestampBegin, s);

        s += 2;

        const auto* textBegin = s;
        while (*s && *s != '\f')
            ++s;

        e.text.assign(textBegin, s);

        entries.push_back(std::move(e));

        validDataSize = s - data;

        if (*s == '\f') {
            if (!s[1])
                // Truncated \f\n terminator.
                break;

            if (s[1] != '\n') {
                setError(
                    "Unexpected {:#04x} instead of \\n at {} for "
                    "text at {}",
                    s[1], s - data, textBegin - data);
                return false;
            }

            if (!s[2])
                // \f\n at the end of the file is a truncated
                // subsequent entry.
                break;

            s += 2;
        }
    }

    return true;
}


static os::StdFileUPtr openSync(
    const char* filePath, const char* mode)
{
    os::StdFileUPtr fp{os::fopen(filePath, mode)};
    if (!fp) {
        setError(
            "os::fopen(..., \"{}\"): {}",
            mode, os::getErrnoMsg(errno));
        return nullptr;
    }

    const auto fileDir = os::getDirName(filePath);

    try {
        os::syncDir(fileDir.empty() ? "." : fileDir.c_str());
    } catch (os::Error& e) {
        setError("os::syncDir(): {}", e.what());
        return nullptr;
    }

    return fp;
}


DpsoHistory* dpsoHistoryOpen(const char* filePath)
{
    auto history = std::make_unique<DpsoHistory>();
    history->filePath = filePath;

    std::string data;
    std::size_t validDataSize{};

    if (!loadData(filePath, data)
            || !createEntries(
                data.c_str(), history->entries, validDataSize))
        return nullptr;

    if (validDataSize != data.size())
        // Don't open the file to "restore" the error state.
        return history.release();

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


bool dpsoHistoryAppend(
    DpsoHistory* history, const DpsoHistoryEntry* entry)
{
    if (!history) {
        setError("history is null");
        return false;
    }

    if (!entry) {
        setError("entry is null");
        return false;
    }

    if (!history->fp) {
        setError("History is in the error state and is read-only");
        return false;
    }

    DpsoHistory::Entry e{entry->timestamp, entry->text};

    std::replace(e.timestamp.begin(), e.timestamp.end(), '\n', ' ');
    std::replace(e.text.begin(), e.text.end(), '\f', ' ');

    auto* fp = history->fp.get();

    try {
        if (!history->entries.empty())
            os::write(fp, "\f\n");

        os::write(fp, e.timestamp);
        os::write(fp, "\n\n");
        os::write(fp, e.text);
    } catch (os::Error& e) {
        setError("os::write(): {}", e.what());
        history->fp.reset();
        return false;
    }

    if (std::fflush(fp) == EOF) {
        setError("fflush() failed");
        history->fp.reset();
        return false;
    }

    try {
        os::syncFile(fp);
    } catch (os::Error& e) {
        setError("os::syncFile(): {}", e.what());
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
        *entry = {"", ""};
        return;
    }

    const auto& e = history->entries[idx];
    *entry = {e.timestamp.c_str(), e.text.c_str()};
}


bool dpsoHistoryClear(DpsoHistory* history)
{
    if (!history) {
        setError("history is null");
        return false;
    }

    // Note that we allow clearing while in the error state.

    history->fp.reset();
    history->entries.clear();

    history->fp = openSync(history->filePath.c_str(), "wb");
    return history->fp != nullptr;
}
