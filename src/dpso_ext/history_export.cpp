
#include "history_export.h"

#include <optional>
#include <vector>

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"
#include "dpso_utils/str.h"
#include "dpso_utils/stream/file_stream.h"
#include "dpso_utils/stream/out_newline_conversion_stream.h"
#include "dpso_utils/stream/utils.h"


using namespace dpso;


namespace {


void writePlainText(Stream& stream, const DpsoHistory* history)
{
    for (int i = 0; i < dpsoHistoryCount(history); ++i) {
        if (i > 0)
            write(stream, "\n\n\n");

        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        write(stream, "=== ");
        write(stream, e.timestamp);
        write(stream, " ===\n\n");
        write(stream, e.text);
    }

    write(stream, '\n');
}


void writeEscapedHtml(
    Stream& stream, const char* indent, const char* text)
{
    for (const auto* s = text; *s;) {
        write(stream, indent);

        while (*s) {
            const auto c = *s++;

            switch (c) {
            case '\n':
                // Although a trailing <br> has no effect (it doesn't
                // add an empty line when rendered in browsers), we
                // still add it so that we can restore the original
                // text from the resulting HTML.
                write(stream, "<br>");
                break;
            case '<':
                write(stream, "&lt;");
                break;
            case '>':
                write(stream, "&gt;");
                break;
            case '&':
                write(stream, "&amp;");
                break;
            default:
                write(stream, c);
                break;
            }

            if (c == '\n') {
                write(stream, c);
                break;
            }
        }
    }
}


// W3C Markup Validator: https://validator.w3.org/
void writeHtml(Stream& stream, const DpsoHistory* history)
{
    write(
        stream,
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
            write(stream, "  <hr>\n");

        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        write(stream, "  <p class=\"timestamp\"><b>");
        writeEscapedHtml(stream, "", e.timestamp);
        write(stream, "</b></p>\n");

        write(stream, "  <p class=\"text\">\n");
        writeEscapedHtml(stream, "    ", e.text);
        write(stream, "\n  </p>\n");
    }

    write(
        stream,
        "</body>\n"
        "</html>\n");
}


void writeEscapedJson(Stream& stream, const char* text)
{
    for (const auto* s = text; *s; ++s)
        switch (const auto c = *s) {
        case '\b':
            write(stream, "\\b");
            break;
        case '\f':
            write(stream, "\\f");
            break;
        case '\n':
            write(stream, "\\n");
            break;
        case '\r':
            write(stream, "\\r");
            break;
        case '\t':
            write(stream, "\\t");
            break;
        default:
            if (c == '\\' || c == '/' || c == '"')
                write(stream, '\\');

            write(stream, c);
            break;
        }
}


// To validate JSON:
//   python3 -m json.tool *.json > /dev/null
void writeJson(Stream& stream, const DpsoHistory* history)
{
    write(stream, "[\n");

    for (int i = 0; i < dpsoHistoryCount(history); ++i) {
        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        write(
            stream,
            "  {\n"
            "    \"timestamp\": \"");
        writeEscapedJson(stream, e.timestamp);
        write(stream, "\",\n");

        write(stream, "    \"text\": \"");
        writeEscapedJson(stream, e.text);
        write(
            stream,
            "\"\n"
            "  }");

        if (i + 1 < dpsoHistoryCount(history))
            write(stream, ',');
        write(stream, '\n');
    }

    write(stream, "]\n");
}


struct ExportFormatInfo {
    using WriteFn = void (&)(Stream&, const DpsoHistory*);

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

    std::optional<FileStream> file;
    try {
        file.emplace(filePath, FileStream::Mode::write);
    } catch (os::Error& e) {
        setError("FileStream(..., Mode::write): {}", e.what());
        return false;
    }

    // None of the export formats require a particular line ending
    // style, so use the OS newline to make Windows Notepad users
    // happy.
    OutNewlineConversionStream newlineConversionStream{*file};

    try {
        exportFormatInfos[exportFormat].writeFn(
            newlineConversionStream, history);
    } catch (StreamError& e) {
        setError("{}", e.what());
        return false;
    }

    return true;
}
