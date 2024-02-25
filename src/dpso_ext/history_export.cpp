
#include "history_export.h"

#include <optional>
#include <vector>

#include "dpso_utils/error_set.h"
#include "dpso_utils/file.h"
#include "dpso_utils/os.h"
#include "dpso_utils/str.h"


using namespace dpso;


namespace {


void writePlainText(File& file, const DpsoHistory* history)
{
    for (int i = 0; i < dpsoHistoryCount(history); ++i) {
        if (i > 0)
            write(
                file,
                DPSO_OS_NEWLINE DPSO_OS_NEWLINE DPSO_OS_NEWLINE);

        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        write(file, "=== ");
        write(file, str::lfToNativeNewline(e.timestamp));
        write(file, " ===" DPSO_OS_NEWLINE DPSO_OS_NEWLINE);
        write(file, str::lfToNativeNewline(e.text));
    }

    write(file, DPSO_OS_NEWLINE);
}


void writeEscapedHtml(
    File& file, const char* indent, const char* text)
{
    for (const auto* s = text; *s;) {
        write(file, indent);

        while (*s) {
            const auto c = *s++;

            switch (c) {
            case '\n':
                // Although a trailing <br> has no effect (it doesn't
                // add an empty line when rendered in browsers), we
                // still add it so that we can restore the original
                // text from the resulting HTML.
                write(file, "<br>");
                break;
            case '<':
                write(file, "&lt;");
                break;
            case '>':
                write(file, "&gt;");
                break;
            case '&':
                write(file, "&amp;");
                break;
            default:
                write(file, c);
                break;
            }

            if (c == '\n') {
                write(file, DPSO_OS_NEWLINE);
                break;
            }
        }
    }
}


// W3C Markup Validator: https://validator.w3.org/
void writeHtml(File& file, const DpsoHistory* history)
{
    write(
        file,
        "<!DOCTYPE html>"            DPSO_OS_NEWLINE
        "<html>"                     DPSO_OS_NEWLINE
        "<head>"                     DPSO_OS_NEWLINE
        "  <meta charset=\"utf-8\">" DPSO_OS_NEWLINE
        "  <title>History</title>"   DPSO_OS_NEWLINE
        "  <style>"                  DPSO_OS_NEWLINE
        "    .text {"                DPSO_OS_NEWLINE
        "      margin: 1em 1em 2em;" DPSO_OS_NEWLINE
        "      line-height: 1.6;"    DPSO_OS_NEWLINE
        "    }"                      DPSO_OS_NEWLINE
        "  </style>"                 DPSO_OS_NEWLINE
        "</head>"                    DPSO_OS_NEWLINE
        "<body>"                     DPSO_OS_NEWLINE);

    for (int i = 0; i < dpsoHistoryCount(history); ++i) {
        if (i > 0)
            write(file, "  <hr>" DPSO_OS_NEWLINE);

        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        write(file, "  <p class=\"timestamp\"><b>");
        writeEscapedHtml(file, "", e.timestamp);
        write(file, "</b></p>" DPSO_OS_NEWLINE);

        write(file, "  <p class=\"text\">" DPSO_OS_NEWLINE);
        writeEscapedHtml(file, "    ", e.text);
        write(file, DPSO_OS_NEWLINE "  </p>" DPSO_OS_NEWLINE);
    }

    write(
        file,
        "</body>" DPSO_OS_NEWLINE
        "</html>" DPSO_OS_NEWLINE);
}


void writeEscapedJson(File& file, const char* text)
{
    for (const auto* s = text; *s; ++s)
        switch (const auto c = *s) {
        case '\b':
            write(file, "\\b");
            break;
        case '\f':
            write(file, "\\f");
            break;
        case '\n':
            write(file, "\\n");
            break;
        case '\r':
            write(file, "\\r");
            break;
        case '\t':
            write(file, "\\t");
            break;
        default:
            if (c == '\\' || c == '/' || c == '"')
                write(file, '\\');

            write(file, c);
            break;
        }
}


// To validate JSON:
//   python3 -m json.tool *.json > /dev/null
void writeJson(File& file, const DpsoHistory* history)
{
    write(file, "[" DPSO_OS_NEWLINE);

    for (int i = 0; i < dpsoHistoryCount(history); ++i) {
        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        write(
            file,
            "  {" DPSO_OS_NEWLINE
            "    \"timestamp\": \"");
        writeEscapedJson(file, e.timestamp);
        write(file, "\"," DPSO_OS_NEWLINE);

        write(file, "    \"text\": \"");
        writeEscapedJson(file, e.text);
        write(
            file,
            "\"" DPSO_OS_NEWLINE
            "  }");

        if (i + 1 < dpsoHistoryCount(history))
            write(file, ',');
        write(file, DPSO_OS_NEWLINE);
    }

    write(file, "]" DPSO_OS_NEWLINE);
}


struct ExportFormatInfo {
    using WriteFn = void (&)(File&, const DpsoHistory*);

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

    std::optional<File> file;
    try {
        file.emplace(filePath, File::Mode::write);
    } catch (os::Error& e) {
        setError("File(..., Mode::write): {}", e.what());
        return false;
    }

    try {
        exportFormatInfos[exportFormat].writeFn(*file, history);
    } catch (os::Error& e) {
        setError("{}", e.what());
        return false;
    }

    return true;
}
