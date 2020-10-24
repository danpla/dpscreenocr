
#include "history_export.h"

#include <cstdio>
#include <cstring>

#include "dpso/str.h"
#include "history.h"
#include "os.h"


static void exportPlainText(std::FILE* fp)
{
    for (int i = 0; i < dpsoHistoryCount(); ++i) {
        if (i > 0)
            std::fputs("\n\n", fp);

        DpsoHistoryEntry e;
        dpsoHistoryGet(i, &e);

        std::fputs("=== ", fp);
        std::fputs(e.timestamp, fp);
        std::fputs(" ===\n\n", fp);

        std::fputs(e.text, fp);
    }
}


// W3C Markup Validator: https://validator.w3.org/
static void exportHtml(std::FILE* fp)
{
    std::fputs(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "  <head>\n"
        "    <meta charset=\"utf-8\">\n"
        "    <title>History</title>\n"
        "    <style>\n"
        "      .text {\n"
        "         margin-left: 2em;\n"
        "         margin-bottom: 2em;\n"
        "         line-height: 1.6;\n"
        "       }\n"
        "    </style>\n"
        "  </head>\n"
        "  <body>\n",
        fp);

    for (int i = 0; i < dpsoHistoryCount(); ++i) {
        if (i > 0)
            std::fputs("    <hr>\n", fp);

        DpsoHistoryEntry e;
        dpsoHistoryGet(i, &e);

        std::fprintf(
            fp,
            "    <p class=\"timestamp\"><b>%s</b></p>\n",
            e.timestamp);

        std::fputs("    <p class=\"text\">\n", fp);

        const auto* s = e.text;
        while (*s) {
            std::fputs("      ", fp);

            while (*s) {
                const auto c = *s;
                ++s;

                switch (c) {
                    case '\n':
                        std::fputs("<br>\n", fp);
                        break;
                    case '<':
                        std::fputs("&lt;", fp);
                        break;
                    case '>':
                        std::fputs("&gt;", fp);
                        break;
                    case '"':
                        std::fputs("&quot;", fp);
                        break;
                    case '\'':
                        std::fputs("&apos;", fp);
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

        std::fputs("    </p>\n", fp);
    }

    std::fputs(
        "  </body>\n"
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

        std::fputs(
            "  {\n"
            "    \"timestamp\": \"",
            fp);
        std::fputs(e.timestamp, fp);
        std::fputs("\",\n", fp);

        std::fputs("    \"text\": \"", fp);
        const auto* s = e.text;
        while (*s) {
            const auto c = *s;
            ++s;

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


void dpsoHistoryExport(
    const char* fileName, DpsoHistoryExportFormat exportFormat)
{
    // We intentionally use fopen() without 'b' flag, enabling CRLF
    // line endings on Windows. This is not required by any export
    // format, but is convenient for Notepad users.
    auto* fp = dpso::fopenUtf8(fileName, "w");
    if (!fp)
        return;

    switch (exportFormat) {
        case dpsoHistoryExportFormatPlainText:
            exportPlainText(fp);
            break;
        case dpsoHistoryExportFormatHtml:
            exportHtml(fp);
            break;
        case dpsoHistoryExportFormatJson:
            exportJson(fp);
            break;
        case dpsoNumHistoryExportFormats:
            break;
    }

    std::fclose(fp);
}


static DpsoHistoryExportFormat exportFormatFromFileName(
    const char* fileName)
{
    struct Extension {
        const char* str;
        DpsoHistoryExportFormat format;
    };

    // Plain text is not listed here as it's a fallback format
    static const Extension extensions[] = {
        {".html", dpsoHistoryExportFormatHtml},
        {".htm", dpsoHistoryExportFormatHtml},
        {".json", dpsoHistoryExportFormatJson},
    };

    const auto* ext = std::strrchr(fileName, '.');
    if (ext && (ext == fileName || ext[-1] == '/'))
        // A leading period denotes a "hidden" file on Unix-like
        // systems. We follow this convention on all platforms.
        ext = nullptr;

    if (ext)
        for (const auto& e : extensions)
            if (dpso::str::cmpIc(ext, e.str) == 0)
                return e.format;

    return dpsoHistoryExportFormatPlainText;
}


void dpsoHistoryExportAuto(const char* fileName)
{
    dpsoHistoryExport(fileName, exportFormatFromFileName(fileName));
}
