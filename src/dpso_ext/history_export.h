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
 * Detect history export format by the file name extension.
 *
 * The function returns true if the file name has an extension that
 * matches a supported export format. If exportFormat is not null, it
 * is set to the detected format.
 *
 * Returns false and leaves exportFormat unchanged if the file name
 * has no extension or if the extension does not match a supported
 * export format.
 *
 * The extension check is case-insensitive.
 */
bool dpsoHistoryDetectExportFormat(
    const char* filePath, DpsoHistoryExportFormat* exportFormat);


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
