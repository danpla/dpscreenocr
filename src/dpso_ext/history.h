
#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct DpsoHistoryEntry {
    const char* timestamp;
    const char* text;
} DpsoHistoryEntry;


/**
 * History.
 *
 * Editing the history will result in immediate modification of the
 * underlying file. In case of IO error, the history is set to the
 * failure state: it becomes read-only, and all further modification
 * attempts will be rejected.
 */
typedef struct DpsoHistory DpsoHistory;


/**
 * Open history.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * null. Nonexistent filePath is not considered an error; the file
 * will be created in this case.
 */
DpsoHistory* dpsoHistoryOpen(const char* filePath);


void dpsoHistoryClose(DpsoHistory* history);


/**
 * Get the number of history entries.
 */
int dpsoHistoryCount(const DpsoHistory* history);


/**
 * Append history entry.
 *
 * Line feeds (\n) in the timestamp and form feeds (\f) in the text
 * will be replaced by spaces.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false. Reasons include:
 *   * history or entry is null
 *   * IO error
 *   * History is in failure state
 */
bool dpsoHistoryAppend(
    DpsoHistory* history, const DpsoHistoryEntry* entry);


/**
 * Get history entry.
 *
 * The function fills the entry with pointers to strings that remain
 * valid till the next call to a routine that modifies the history,
 * like dpsoHistoryAppend() or dpsoHistoryClear().
 */
void dpsoHistoryGet(
    const DpsoHistory* history, int idx, DpsoHistoryEntry* entry);


/**
 * Clear the history.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false. Reasons include:
 *   * history is null
 *   * IO error
 *   * History is in failure state
 */
bool dpsoHistoryClear(DpsoHistory* history);


#ifdef __cplusplus
}


#include <memory>


namespace dpso {


struct HistoryCloser {
    void operator()(DpsoHistory* history) const
    {
        dpsoHistoryClose(history);
    }
};


using HistoryUPtr = std::unique_ptr<DpsoHistory, HistoryCloser>;


}


#endif
