
#include "history_export.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <vector>

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"
#include "dpso_utils/str.h"


using namespace dpso;


namespace {


void writePlainText(std::FILE* fp, const DpsoHistory* history)
{
    for (int i = 0; i < dpsoHistoryCount(history); ++i) {
        if (i > 0)
            os::write(fp, "\n\n\n");

        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        os::write(fp, "=== ");
        os::write(fp, e.timestamp);
        os::write(fp, " ===\n\n");
        os::write(fp, e.text);
    }

    os::write(fp, "\n");
}


void writeEscapedHtml(
    std::FILE* fp, const char* indent, const char* text)
{
    for (const auto* s = text; *s;) {
        os::write(fp, indent);

        while (*s) {
            const auto c = *s++;

            switch (c) {
            case '\n':
                // Although a trailing <br> has no effect (it doesn't
                // add an empty line when rendered in browsers), we
                // still add it so that we can restore the original
                // text from the resulting HTML.
                os::write(fp, "<br>");
                break;
            case '<':
                os::write(fp, "&lt;");
                break;
            case '>':
                os::write(fp, "&gt;");
                break;
            case '&':
                os::write(fp, "&amp;");
                break;
            default:
                os::write(fp, c);
                break;
            }

            if (c == '\n') {
                os::write(fp, c);
                break;
            }
        }
    }
}


// W3C Markup Validator: https://validator.w3.org/
void writeHtml(std::FILE* fp, const DpsoHistory* history)
{
    os::write(
        fp,
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
        "<body>\n");

    for (int i = 0; i < dpsoHistoryCount(history); ++i) {
        if (i > 0)
            os::write(fp, "  <hr>\n");

        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        os::write(fp, "  <p class=\"timestamp\"><b>");
        writeEscapedHtml(fp, "", e.timestamp);
        os::write(fp, "</b></p>\n");

        os::write(fp, "  <p class=\"text\">\n");
        writeEscapedHtml(fp, "    ", e.text);
        os::write(fp, "\n  </p>\n");
    }

    os::write(
        fp,
        "</body>\n"
        "</html>\n");
}


void writeEscapedJson(std::FILE* fp, const char* text)
{
    for (const auto* s = text; *s; ++s)
        switch (const auto c = *s) {
        case '\b':
            os::write(fp, "\\b");
            break;
        case '\f':
            os::write(fp, "\\f");
            break;
        case '\n':
            os::write(fp, "\\n");
            break;
        case '\r':
            os::write(fp, "\\r");
            break;
        case '\t':
            os::write(fp, "\\t");
            break;
        default:
            if (c == '\\' || c == '/' || c == '"')
                os::write(fp, '\\');

            os::write(fp, c);
            break;
        }
}


// To validate JSON:
//   python3 -m json.tool *.json > /dev/null
void writeJson(std::FILE* fp, const DpsoHistory* history)
{
    os::write(fp, "[\n");

    for (int i = 0; i < dpsoHistoryCount(history); ++i) {
        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        os::write(fp,
            "  {\n"
            "    \"timestamp\": \"");
        writeEscapedJson(fp, e.timestamp);
        os::write(fp, "\",\n");

        os::write(fp, "    \"text\": \"");
        writeEscapedJson(fp, e.text);
        os::write(fp,
            "\"\n"
            "  }");

        if (i + 1 < dpsoHistoryCount(history))
            os::write(fp, ',');
        os::write(fp, '\n');
    }

    os::write(fp, "]\n");
}


struct ExportFormatInfo {
    using WriteFn = void (&)(std::FILE*, const DpsoHistory*);

    const char* name;
    std::vector<const char*> extensions;
    WriteFn writeFn;
};


const ExportFormatInfo exportFormatInfos[] = {
    {"TXT", {".txt"}, writePlainText},
    {"HTML", {".html", ".htm"}, writeHtml},
    {"JSON", {".json"}, writeJson},
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
        static_cast<int>(info.extensions.size())};
}


DpsoHistoryExportFormat dpsoHistoryDetectExportFormat(
    const char* filePath,
    DpsoHistoryExportFormat defaultExportFormat)
{
    const auto* ext = os::getFileExt(filePath);
    if (!ext)
        return defaultExportFormat;

    for (int i = 0; i < dpsoNumHistoryExportFormats; ++i)
        for (const auto* formatExt : exportFormatInfos[i].extensions)
            if (str::cmp(ext, formatExt, str::cmpIgnoreCase) == 0)
                return static_cast<DpsoHistoryExportFormat>(i);

    return defaultExportFormat;
}


bool dpsoHistoryExport(
    const DpsoHistory* history,
    const char* filePath,
    DpsoHistoryExportFormat exportFormat)
{
    if (!history) {
        setError("history is null");
        return false;
    }

    if (exportFormat < 0
            || exportFormat >= dpsoNumHistoryExportFormats) {
        setError(
            "Unknown export format {}",
            static_cast<int>(exportFormat));
        return false;
    }

    // We intentionally use fopen() without 'b' flag, enabling CRLF
    // line endings on Windows. This is not required by any export
    // format, but is convenient for Notepad users.
    os::StdFileUPtr fp{os::fopen(filePath, "w")};
    if (!fp) {
        setError("os::fopen(..., \"w\"): {}", std::strerror(errno));
        return false;
    }

    try {
        exportFormatInfos[exportFormat].writeFn(fp.get(), history);
    } catch (os::Error& e) {
        setError("{}", e.what());
        return false;
    }

    return true;
}
