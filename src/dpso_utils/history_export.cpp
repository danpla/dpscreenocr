
#include "history_export.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "dpso/error.h"
#include "dpso/str.h"
#include "history.h"
#include "os.h"
#include "os_cpp.h"


DpsoHistoryExportFormat dpsoHistoryDetectExportFormat(
    const char* fileName,
    DpsoHistoryExportFormat defaultExportFormat)
{
    struct Extension {
        const char* str;
        DpsoHistoryExportFormat format;
    };

    static const Extension extensions[] = {
        {".txt", dpsoHistoryExportFormatPlainText},
        {".html", dpsoHistoryExportFormatHtml},
        {".htm", dpsoHistoryExportFormatHtml},
        {".json", dpsoHistoryExportFormatJson},
    };

    if (const auto* ext = dpsoGetFileExt(fileName))
        for (const auto& e : extensions)
            if (dpso::str::cmp(
                    ext, e.str, dpso::str::cmpIgnoreCase) == 0)
                return e.format;

    return defaultExportFormat;
}


static void exportPlainText(std::FILE* fp)
{
    for (int i = 0; i < dpsoHistoryCount(); ++i) {
        if (i > 0)
            std::fputs("\n\n", fp);

        DpsoHistoryEntry e;
        dpsoHistoryGet(i, &e);

        std::fprintf(fp, "=== %s ===\n\n", e.timestamp);
        std::fputs(e.text, fp);
    }
}


// W3C Markup Validator: https://validator.w3.org/
static void exportHtml(std::FILE* fp)
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

    for (int i = 0; i < dpsoHistoryCount(); ++i) {
        if (i > 0)
            std::fputs("  <hr>\n", fp);

        DpsoHistoryEntry e;
        dpsoHistoryGet(i, &e);

        std::fprintf(
            fp,
            "  <p class=\"timestamp\"><b>%s</b></p>\n",
            e.timestamp);

        std::fputs("  <p class=\"text\">\n", fp);

        const auto* s = e.text;
        while (*s) {
            std::fputs("    ", fp);

            while (*s) {
                const auto c = *s++;

                switch (c) {
                    case '\n':
                        // Trailing <br> has no effect; it doesn't add
                        // an empty line when rendering in browsers.
                        if (*s)
                            std::fputs("<br>", fp);
                        std::putc('\n', fp);
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

                if (c == '\n')
                    break;
            }
        }

        std::fputs("  </p>\n", fp);
    }

    std::fputs(
        "</body>\n"
        "</html>\n",
        fp);
}


// To validate JSON:
//   python3 -m json.tool *.json > /dev/null
static void exportJson(std::FILE* fp)
{
    std::fputs("[\n", fp);

    for (int i = 0; i < dpsoHistoryCount(); ++i) {
        DpsoHistoryEntry e;
        dpsoHistoryGet(i, &e);

        std::fprintf(
            fp,
            "  {\n"
            "    \"timestamp\": \"%s\",\n",
            e.timestamp);

        std::fputs("    \"text\": \"", fp);
        const auto* s = e.text;
        while (*s) {
            const auto c = *s++;

            switch (c) {
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
        std::fputs(
            "\"\n"
            "  }",
            fp);
        if (i + 1 < dpsoHistoryCount())
            std::fputc(',', fp);
        std::fputc('\n', fp);
    }

    std::fputs("]\n", fp);
}


int dpsoHistoryExport(
    const char* fileName, DpsoHistoryExportFormat exportFormat)
{
    // We intentionally use fopen() without 'b' flag, enabling CRLF
    // line endings on Windows. This is not required by any export
    // format, but is convenient for Notepad users.
    dpso::StdFileUPtr fp{dpsoFopenUtf8(fileName, "w")};
    if (!fp) {
        dpsoSetError(
            "dpsoFopenUtf8(..., \"w\") failed: %s",
            std::strerror(errno));
        return false;
    }

    switch (exportFormat) {
        case dpsoHistoryExportFormatPlainText:
            exportPlainText(fp.get());
            break;
        case dpsoHistoryExportFormatHtml:
            exportHtml(fp.get());
            break;
        case dpsoHistoryExportFormatJson:
            exportJson(fp.get());
            break;
        case dpsoNumHistoryExportFormats:
            break;
    }

    return true;
}
