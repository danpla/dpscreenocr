#pragma once

#include <stdbool.h>

#include "history.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    dpsoHistoryExportFormatPlainText,
    dpsoHistoryExportFormatHtml,
    dpsoHistoryExportFormatJson,
    dpsoNumHistoryExportFormats
} DpsoHistoryExportFormat;


typedef struct DpsoHistoryExportFormatInfo {
    const char* name;

    /**
     * Array of one or more file extensions.
     *
     * Each extension is in lower case and has a leading period.
     */
    const char* const* extensions;

    /**
     * Number of items in the extensions array.
     *
     * Always > 0.
     */
    int numExtensions;
} DpsoHistoryExportFormatInfo;


void dpsoHistoryGetExportFormatInfo(
    DpsoHistoryExportFormat exportFormat,
    DpsoHistoryExportFormatInfo* exportFormatInfo);


/**
 * Detect history export format by file name extension.
 *
 * Returns defaultExportFormat if filePath has no extension or if the
 * extension is not known. The function is case-insensitive.
 */
DpsoHistoryExportFormat dpsoHistoryDetectExportFormat(
    const char* filePath,
    DpsoHistoryExportFormat defaultExportFormat);


/**
 * Export history to a file.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool dpsoHistoryExport(
    const DpsoHistory* history,
    const char* filePath,
    DpsoHistoryExportFormat exportFormat);


#ifdef __cplusplus
}
#endif
