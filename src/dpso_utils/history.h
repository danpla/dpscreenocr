
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
 * The function clears the history and then loads the historyName file
 * for application appName. appName is a subdirectory in a platform-
 * specific configuration dir. historyName should be a base name.
 */
void dpsoHistoryLoad(const char* appName, const char* historyName);

/**
 * Save history.
 *
 * \sa dpsoHistoryLoad()
 */
void dpsoHistorySave(const char* appName, const char* historyName);


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
