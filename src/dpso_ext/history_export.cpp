#include "history_export.h"

#include <optional>
#include <string_view>
#include <vector>

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"
#include "dpso_utils/str.h"
#include "dpso_utils/stream/file_stream.h"
#include "dpso_utils/stream/out_newline_conversion_stream.h"
#include "dpso_utils/stream/utils.h"


using namespace dpso;


namespace {


struct CharReplacement {
    char from;
    std::string_view to;
};


template<std::size_t N>
void write(
    Stream& stream, char c, const CharReplacement (&replacements)[N])
{
    for (const auto& r : replacements)
        if (c == r.from) {
            write(stream, r.to);
            return;
        }

    write(stream, c);
}


void writePlainText(Stream& stream, const DpsoHistory* history)
{
    for (int i{}; i < dpsoHistoryCount(history); ++i) {
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
    Stream& stream, int indentSize, const char* text)
{
    static const CharReplacement replacements[]{
        {'\n', "<br>\n"},
        {'<', "&lt;"},
        {'>', "&gt;"},
        {'&', "&amp;"},
    };

    for (const auto* s = text; *s;) {
        for (int i{}; i < indentSize; ++i)
            write(stream, ' ');

        while (*s) {
            const auto c = *s++;
            write(stream, c, replacements);

            if (c == '\n')
                break;
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
        "    .timestamp {\n"
        "      font-weight: bold;\n"
        "    }\n"
        "    .text {\n"
        "      margin: 1em 1em 2em;\n"
        "      line-height: 1.6;\n"
        "    }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n");

    for (int i{}; i < dpsoHistoryCount(history); ++i) {
        if (i > 0)
            write(stream, "  <hr>\n");

        DpsoHistoryEntry e;
        dpsoHistoryGet(history, i, &e);

        write(stream, "  <p class=\"timestamp\">");
        writeEscapedHtml(stream, 0, e.timestamp);
        write(stream, "</p>\n");

        write(stream, "  <p class=\"text\">\n");
        writeEscapedHtml(stream, 4, e.text);
        write(stream, "\n  </p>\n");
    }

    write(
        stream,
        "</body>\n"
        "</html>\n");
}


void writeEscapedJson(Stream& stream, const char* text)
{
    static const CharReplacement replacements[]{
        {'\b', "\\b"},
        {'\f', "\\f"},
        {'\n', "\\n"},
        {'\r', "\\r"},
        {'\t', "\\t"},
        {'\\', "\\\\"},
        {'/',  "\\/"},
        {'\"', "\\\""},
    };

    for (const auto* s = text; *s; ++s)
        write(stream, *s, replacements);
}


// To validate JSON:
//   python3 -m json.tool *.json > /dev/null
void writeJson(Stream& stream, const DpsoHistory* history)
{
    write(stream, "[\n");

    for (int i{}; i < dpsoHistoryCount(history); ++i) {
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


const ExportFormatInfo exportFormatInfos[]{
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
    const auto ext = os::getFileExt(filePath);

    for (int i{}; i < dpsoNumHistoryExportFormats; ++i)
        for (const auto* formatExt : exportFormatInfos[i].extensions)
            if (str::equalIgnoreCase(ext, formatExt))
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
    OutNewlineConversionStream newlineConversionStream{
        *file, os::newline};

    try {
        exportFormatInfos[exportFormat].writeFn(
            newlineConversionStream, history);
    } catch (StreamError& e) {
        setError("{}", e.what());
        return false;
    }

    return true;
}
