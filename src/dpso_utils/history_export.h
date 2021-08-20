
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
 * Export history to a file.
 */
void dpsoHistoryExport(
    const char* fileName, DpsoHistoryExportFormat exportFormat);


/**
 * Export history to a file automatically detecting export format.
 *
 * This function is the same as dpsoHistoryExport(), but the export
 * format is automatically detected by the file name extension,
 * ignoring case. If fileName has no extension or if the extension
 * is not known, plain text is used.
 */
void dpsoHistoryExportAuto(const char* fileName);


#ifdef __cplusplus
}
#endif
