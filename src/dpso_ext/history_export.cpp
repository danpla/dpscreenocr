
#include "history_export.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <vector>

#include "dpso_utils/error.h"
#include "dpso_utils/os.h"
#include "dpso_utils/str.h"


namespace {


void exportPlainText(const DpsoHistory* history, std::FILE* fp)
{
    for (int i = 0; i < dpsoHistoryCount(history); ++i) {
        if (i > 0)
            std::fputs("\n\n\n", fp);

        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        std::fprintf(fp, "=== %s ===\n\n", e.timestamp);
        std::fputs(e.text, fp);
    }

    std::fputs("\n", fp);
}


void writeEscapedHtml(
    std::FILE* fp, const char* indent, const char* text)
{
    for (const auto* s = text; *s;) {
        std::fputs(indent, fp);

        while (*s) {
            const auto c = *s++;

            switch (c) {
            case '\n':
                // Although a trailing <br> has no effect (it doesn't
                // add an empty line when rendered in browsers), we
                // still add it so that we can restore the original
                // text from the resulting HTML.
                std::fputs("<br>", fp);
                break;
            case '<':
                std::fputs("&lt;", fp);
                break;
            case '>':
                std::fputs("&gt;", fp);
                break;
            case '&':
                std::fputs("&amp;", fp);
                break;
            default:
                std::fputc(c, fp);
                break;
            }

            if (c == '\n') {
                std::putc(c, fp);
                break;
            }
        }
    }
}


// W3C Markup Validator: https://validator.w3.org/
void exportHtml(const DpsoHistory* history, std::FILE* fp)
{
    std::fputs(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <title>History</title>\n"
        "  <style>\n"
        "    .text {\n"
        "      margin: 1em 1em 2em;\n"
        "      line-height: 1.6;\n"
        "    }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n",
        fp);

    for (int i = 0; i < dpsoHistoryCount(history); ++i) {
        if (i > 0)
            std::fputs("  <hr>\n", fp);

        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        std::fputs("  <p class=\"timestamp\"><b>", fp);
        writeEscapedHtml(fp, "", e.timestamp);
        std::fputs("</b></p>\n", fp);

        std::fputs("  <p class=\"text\">\n", fp);
        writeEscapedHtml(fp, "    ", e.text);
        std::fputs("\n  </p>\n", fp);
    }

    std::fputs(
        "</body>\n"
        "</html>\n",
        fp);
}


void writeEscapedJson(std::FILE* fp, const char* text)
{
    for (const auto* s = text; *s; ++s) {
        switch (const auto c = *s) {
        case '\b':
            std::fputs("\\b", fp);
            break;
        case '\f':
            std::fputs("\\f", fp);
            break;
        case '\n':
            std::fputs("\\n", fp);
            break;
        case '\r':
            std::fputs("\\r", fp);
            break;
        case '\t':
            std::fputs("\\t", fp);
            break;
        default:
            if (c == '\\' || c == '/' || c == '"')
                std::fputc('\\', fp);
            std::fputc(c, fp);
            break;
        }
    }
}


// To validate JSON:
//   python3 -m json.tool *.json > /dev/null
void exportJson(const DpsoHistory* history, std::FILE* fp)
{
    std::fputs("[\n", fp);

    for (int i = 0; i < dpsoHistoryCount(history); ++i) {
        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        std::fputs(
            "  {\n"
            "    \"timestamp\": \"",
            fp);
        writeEscapedJson(fp, e.timestamp);
        std::fputs("\",\n", fp);

        std::fputs("    \"text\": \"", fp);
        writeEscapedJson(fp, e.text);
        std::fputs(
            "\"\n"
            "  }",
            fp);

        if (i + 1 < dpsoHistoryCount(history))
            std::fputc(',', fp);
        std::fputc('\n', fp);
    }

    std::fputs("]\n", fp);
}


using ExportFn = void (&)(const DpsoHistory*, std::FILE*);


struct ExportFormatInfo {
    const char* name;
    std::vector<const char*> extensions;
    ExportFn exportFn;
};


const ExportFormatInfo exportFormatInfos[] = {
    {"TXT", {".txt"}, exportPlainText},
    {"HTML", {".html", ".htm"}, exportHtml},
    {"JSON", {".json"}, exportJson},
};
static_assert(
    std::size(exportFormatInfos) == dpsoNumHistoryExportFormats);


}


void dpsoHistoryGetExportFormatInfo(
    DpsoHistoryExportFormat exportFormat,
    DpsoHistoryExportFormatInfo* exportFormatInfo)
{
    if (!exportFormatInfo)
        return;

    if (exportFormat < 0
            || exportFormat >= dpsoNumHistoryExportFormats) {
        static const char* emptyExt = "";
        *exportFormatInfo = {"", &emptyExt, 1};
        return;
    }

    const auto& info = exportFormatInfos[exportFormat];

    *exportFormatInfo = {
        info.name,
        info.extensions.data(),
        static_cast<int>(info.extensions.size())
    };
}


DpsoHistoryExportFormat dpsoHistoryDetectExportFormat(
    const char* filePath,
    DpsoHistoryExportFormat defaultExportFormat)
{
    const auto* ext = dpso::os::getFileExt(filePath);
    if (!ext)
        return defaultExportFormat;

    for (int i = 0; i < dpsoNumHistoryExportFormats; ++i)
        for (const auto* formatExt : exportFormatInfos[i].extensions)
            if (dpso::str::cmp(
                    ext, formatExt, dpso::str::cmpIgnoreCase) == 0)
                return static_cast<DpsoHistoryExportFormat>(i);

    return defaultExportFormat;
}


bool dpsoHistoryExport(
    const DpsoHistory* history,
    const char* filePath,
    DpsoHistoryExportFormat exportFormat)
{
    if (!history) {
        dpsoSetError("history is null");
        return false;
    }

    if (exportFormat < 0
            || exportFormat >= dpsoNumHistoryExportFormats) {
        dpsoSetError("Unknown export format %i", exportFormat);
        return false;
    }

    // We intentionally use fopen() without 'b' flag, enabling CRLF
    // line endings on Windows. This is not required by any export
    // format, but is convenient for Notepad users.
    dpso::os::StdFileUPtr fp{dpso::os::fopen(filePath, "w")};
    if (!fp) {
        dpsoSetError(
            "os::fopen(..., \"w\"): %s", std::strerror(errno));
        return false;
    }

    exportFormatInfos[exportFormat].exportFn(history, fp.get());

    return true;
}
