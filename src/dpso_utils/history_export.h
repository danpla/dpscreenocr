
/**
 * \file
 * History export
 */

#pragma once


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
 * Returns defaultExportFormat if fileName has no extension or if the
 * extension is not known. The function is case-insensitive.
 */
DpsoHistoryExportFormat dpsoHistoryDetectExportFormat(
    const char* fileName,
    DpsoHistoryExportFormat defaultExportFormat);


/**
 * Export history to a file.
 */
void dpsoHistoryExport(
    const char* fileName, DpsoHistoryExportFormat exportFormat);


#ifdef __cplusplus
}
#endif
