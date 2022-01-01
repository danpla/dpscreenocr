
/**
 * \file
 * History handling
 */

#pragma once


#ifdef __cplusplus
extern "C" {
#endif


struct DpsoHistoryEntry {
    const char* timestamp;
    const char* text;
};


/**
 * History.
 *
 * Editing the history will result in immediate modification of the
 * underlying file. In case of IO error, the history is set to the
 * failure state: it becomes read-only, and all further modification
 * attempts will be rejected.
 */
struct DpsoHistory;


/**
 * Open history.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * null. Nonexistent filePath is not considered an error; the file
 * will be created in this case.
 */
struct DpsoHistory* dpsoHistoryOpen(const char* filePath);


void dpsoHistoryClose(struct DpsoHistory* history);


/**
 * Get the number of history entries.
 */
int dpsoHistoryCount(const struct DpsoHistory* history);


/**
 * Append history entry.
 *
 * Line feeds (\n) in the timestamp and form feeds (\f) in the text
 * will be replaced by spaces.
 *
 * On failure, sets an error message (dpsoGetError()) and returns 0.
 * Reasons include:
 *   * history or entry is null
 *   * IO error
 *   * History is in failure state
 */
int dpsoHistoryAppend(
    struct DpsoHistory* history,
    const struct DpsoHistoryEntry* entry);


/**
 * Get history entry.
 *
 * The function fills the entry with pointers to strings that remain
 * valid till the next call to a routine that modifies the history,
 * like dpsoHistoryAppend() or dpsoHistoryClear().
 */
void dpsoHistoryGet(
    const struct DpsoHistory* history,
    int idx,
    struct DpsoHistoryEntry* entry);


/**
 * Clear the history.
 *
 * On failure, sets an error message (dpsoGetError()) and returns 0.
 * Reasons include:
 *   * history is null
 *   * IO error
 *   * History is in failure state
 */
int dpsoHistoryClear(struct DpsoHistory* history);


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
