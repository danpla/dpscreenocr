
#include "history.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"
#include "dpso_utils/stream/file_stream.h"
#include "dpso_utils/stream/utils.h"


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
    std::optional<FileStream> file;
    std::vector<Entry> entries;
};


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


static void openSync(
    std::optional<FileStream>& file,
    const char* filePath,
    FileStream::Mode mode)
{
    try {
        file.emplace(filePath, mode);
    } catch (os::Error& e) {
        setError("FileStream(..., {}): {}", mode, e.what());
    }

    const auto fileDir = os::getDirName(filePath);

    try {
        os::syncDir(fileDir.empty() ? "." : fileDir.c_str());
    } catch (os::Error& e) {
        setError("os::syncDir(): {}", e.what());
        file.reset();
    }
}


DpsoHistory* dpsoHistoryOpen(const char* filePath)
{
    auto history = std::make_unique<DpsoHistory>();
    history->filePath = filePath;

    std::string data;
    try {
        data = os::loadData(filePath);
    } catch (os::FileNotFoundError&) {
    } catch (os::Error& e) {
        setError("os::loadData(): {}", e.what());
        return nullptr;
    }

    std::size_t validDataSize{};
    if (!createEntries(data.c_str(), history->entries, validDataSize))
        return nullptr;

    if (validDataSize != data.size())
        try {
            os::resizeFile(filePath, validDataSize);
        } catch (os::Error& e) {
            setError("os::resizeFile(): {}", e.what());
            return nullptr;
        }

    openSync(history->file, filePath, FileStream::Mode::append);
    if (!history->file)
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

    if (!history->file) {
        setError("History is in the error state and is read-only");
        return false;
    }

    DpsoHistory::Entry e{entry->timestamp, entry->text};

    std::replace(e.timestamp.begin(), e.timestamp.end(), '\n', ' ');
    std::replace(e.text.begin(), e.text.end(), '\f', ' ');

    auto& file = *history->file;

    try {
        if (!history->entries.empty())
            write(file, "\f\n");

        write(file, e.timestamp);
        write(file, "\n\n");
        write(file, e.text);
    } catch (StreamError& e) {
        setError("write(file, ...): {}", e.what());
        history->file.reset();
        return false;
    }

    try {
        file.sync();
    } catch (os::Error& e) {
        setError("FileStream::sync(): {}", e.what());
        history->file.reset();
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

    history->file.reset();
    history->entries.clear();

    openSync(
        history->file,
        history->filePath.c_str(),
        FileStream::Mode::write);

    return history->file.has_value();
}
