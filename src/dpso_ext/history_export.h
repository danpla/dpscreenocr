
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
