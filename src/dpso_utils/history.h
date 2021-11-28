
/**
 * \file
 * History handling
 */

#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * History entry.
 *
 * For more info about the fields, see DpsoJobResult.
 */
struct DpsoHistoryEntry {
    const char* text;
    const char* timestamp;
};


/**
 * Load history.
 *
 * The function clears the history and then loads the filePath file.
 *
 * On failure, sets an error message (dpsoGetError()) and returns 0.
 * Nonexistent filePath is not considered an error.
 */
int dpsoHistoryLoad(const char* filePath);

/**
 * Save history.
 *
 * On failure, sets an error message (dpsoGetError()) and returns 0.
 *
 * \sa dpsoHistoryLoad()
 */
int dpsoHistorySave(const char* filePath);


/**
 * Get the number of history entries.
 */
int dpsoHistoryCount(void);


/**
 * Append history entry.
 */
void dpsoHistoryAppend(const struct DpsoHistoryEntry* entry);


/**
 * Get history entry.
 *
 * The function fills the entry with pointers to strings that remain
 * valid till the next call to a routine that modifies the history,
 * like dpsoHistoryLoad(), dpsoHistoryAppend(), or
 * dpsoHistoryClear().
 */
void dpsoHistoryGet(int idx, struct DpsoHistoryEntry* entry);


/**
 * Clear the history.
 */
void dpsoHistoryClear(void);


#ifdef __cplusplus
}
#endif
