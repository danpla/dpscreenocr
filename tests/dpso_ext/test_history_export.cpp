
#include <initializer_list>
#include <string>
#include <vector>

#include "dpso_ext/history.h"
#include "dpso_ext/history_export.h"
#include "dpso_utils/error_get.h"
#include "dpso_utils/str.h"

#include "flow.h"
#include "utils.h"


namespace {


std::string toStr(DpsoHistoryExportFormat exportFormat)
{
    DpsoHistoryExportFormatInfo exportFormatInfo;
    dpsoHistoryGetExportFormatInfo(exportFormat, &exportFormatInfo);
    return exportFormatInfo.name;
}


void testDetectExportFormat(
    const char* filePath,
    DpsoHistoryExportFormat defaultExportFormat,
    DpsoHistoryExportFormat expectedExportFormat)
{
    const auto gotExportFormat = dpsoHistoryDetectExportFormat(
        filePath, defaultExportFormat);
    if (gotExportFormat == expectedExportFormat)
        return;

    test::failure(
        "dpsoHistoryDetectExportFormat(\"{}\", {}): "
        "expected {}, got {}\n",
        filePath,
        toStr(defaultExportFormat),
        toStr(expectedExportFormat),
        toStr(gotExportFormat));
}


void testDetectExportFormat()
{
    const struct Test {
        std::vector<const char*> extensions;
        DpsoHistoryExportFormat exportFormat;
        DpsoHistoryExportFormat defaultExportFormat;
    } tests[] = {
        {
            {".txt", ".Txt", ".TXT"},
            dpsoHistoryExportFormatPlainText,
            dpsoHistoryExportFormatHtml
        },
        {
            {".html", ".Html", ".HTML", ".htm", ".Htm", ".HTM"},
            dpsoHistoryExportFormatHtml,
            dpsoHistoryExportFormatPlainText
        },
        {
            {".json", ".Json", ".JSON"},
            dpsoHistoryExportFormatJson,
            dpsoHistoryExportFormatPlainText
        },
    };
    static_assert(std::size(tests) == dpsoNumHistoryExportFormats);

    for (const auto* prefix : {"file", "dir.name/file"})
        for (const auto& test : tests) {
            for (const auto* ext : test.extensions)
                testDetectExportFormat(
                    (std::string{prefix} + ext).c_str(),
                    test.defaultExportFormat,
                    test.exportFormat);

            testDetectExportFormat(
                (std::string{prefix} + ".unknown_ext").c_str(),
                test.defaultExportFormat,
                test.defaultExportFormat);
        }
}


void testExport()
{
    static const auto* htmlBegin =
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
        "<body>\n";

    static const auto* htmlEnd =
        "</body>\n"
        "</html>\n";

    const struct Test {
        const char* description;
        std::vector<DpsoHistoryEntry> entries;
        std::vector<std::string> exportedData;

        Test(
                const char* description,
                std::vector<DpsoHistoryEntry>&& entries,
                const char* txtData,
                const char* htmlBodyData,
                const char* jsonData)
            : description{description}
            , entries{std::move(entries)}
            , exportedData{
                dpso::str::lfToNativeNewline(txtData),
                dpso::str::lfToNativeNewline(htmlBegin)
                    + dpso::str::lfToNativeNewline(htmlBodyData)
                    + dpso::str::lfToNativeNewline(htmlEnd),
                dpso::str::lfToNativeNewline(jsonData)}
        {
            if (exportedData.size() != dpsoNumHistoryExportFormats)
                test::fatalError(
                    "Unexpected number of exportedData entries. "
                    "Seems like some export formats were added or "
                    "removed, so exportedData should be adjusted.\n");
        }
    } tests[] = {
        {
            "No entries",
            {},

            "\n"
            ,
            ""
            ,
            "[\n"
            "]\n"},
        {
            "Empty entry",
            {{"", ""}},

            "===  ===\n\n"
            "\n"
            ,

            "  <p class=\"timestamp\"><b></b></p>\n"
            "  <p class=\"text\">\n"
            "\n"
            "  </p>\n"
            ,

            "[\n"
            "  {\n"
            "    \"timestamp\": \"\",\n"
            "    \"text\": \"\"\n"
            "  }\n"
            "]\n"},
        {
            "Two empty entries",
            {{"", ""}, {"", ""}},

            "===  ===\n\n"
            "\n\n\n"
            "===  ===\n\n"
            "\n"
            ,

            "  <p class=\"timestamp\"><b></b></p>\n"
            "  <p class=\"text\">\n"
            "\n"
            "  </p>\n"
            "  <hr>\n"
            "  <p class=\"timestamp\"><b></b></p>\n"
            "  <p class=\"text\">\n"
            "\n"
            "  </p>\n"
            ,

            "[\n"
            "  {\n"
            "    \"timestamp\": \"\",\n"
            "    \"text\": \"\"\n"
            "  },\n"
            "  {\n"
            "    \"timestamp\": \"\",\n"
            "    \"text\": \"\"\n"
            "  }\n"
            "]\n"},
        {
            "Single entry",
            {{"ts", "text"}}
            ,

            "=== ts ===\n\n"
            "text"
            "\n"
            ,

            "  <p class=\"timestamp\"><b>ts</b></p>\n"
            "  <p class=\"text\">\n"
            "    text\n"
            "  </p>\n"
            ,

            "[\n"
            "  {\n"
            "    \"timestamp\": \"ts\",\n"
            "    \"text\": \"text\"\n"
            "  }\n"
            "]\n"},
        {
            "Two entries",
            {{"ts 1", "text 1"}, {"ts 2", "text 2"}},

            "=== ts 1 ===\n\n"
            "text 1"
            "\n\n\n"
            "=== ts 2 ===\n\n"
            "text 2"
            "\n"
            ,

            "  <p class=\"timestamp\"><b>ts 1</b></p>\n"
            "  <p class=\"text\">\n"
            "    text 1\n"
            "  </p>\n"
            "  <hr>\n"
            "  <p class=\"timestamp\"><b>ts 2</b></p>\n"
            "  <p class=\"text\">\n"
            "    text 2\n"
            "  </p>\n"
            ,

            "[\n"
            "  {\n"
            "    \"timestamp\": \"ts 1\",\n"
            "    \"text\": \"text 1\"\n"
            "  },\n"
            "  {\n"
            "    \"timestamp\": \"ts 2\",\n"
            "    \"text\": \"text 2\"\n"
            "  }\n"
            "]\n"},
        {
            "Multiline text",
            {{"ts line 1\nts line 2", "text line 1\ntext line 2"}},

            "=== ts line 1 ts line 2 ===\n\n"
            "text line 1\n"
            "text line 2"
            "\n"
            ,

            "  <p class=\"timestamp\"><b>ts line 1 ts line 2</b></p>\n"
            "  <p class=\"text\">\n"
            "    text line 1<br>\n"
            "    text line 2\n"
            "  </p>\n"
            ,

            "[\n"
            "  {\n"
            "    \"timestamp\": \"ts line 1 ts line 2\",\n"
            "    \"text\": \"text line 1\\ntext line 2\"\n"
            "  }\n"
            "]\n"},
        {
            "Text with trailing newline",
            {{"ts", "text\n"}},

            "=== ts ===\n\n"
            "text\n"
            "\n"
            ,

            "  <p class=\"timestamp\"><b>ts</b></p>\n"
            "  <p class=\"text\">\n"
            "    text<br>\n"
            "\n"
            "  </p>\n"
            ,

            "[\n"
            "  {\n"
            "    \"timestamp\": \"ts\",\n"
            "    \"text\": \"text\\n\"\n"
            "  }\n"
            "]\n"},
        {
            "HTML entities",
            {{"'\"<>&", "'\"<>&"}},

            "=== '\"<>& ===\n\n"
            "'\"<>&"
            "\n"
            ,

            "  <p class=\"timestamp\"><b>'\"&lt;&gt;&amp;</b></p>\n"
            "  <p class=\"text\">\n"
            "    '\"&lt;&gt;&amp;\n"
            "  </p>\n"
            ,

            "[\n"
            "  {\n"
            "    \"timestamp\": \"'\\\"<>&\",\n"
            "    \"text\": \"'\\\"<>&\"\n"
            "  }\n"
            "]\n"},
        {
            "JSON escape sequences",
            // \320\264 is Д in UTF-8
            {{"\b\f\n\r\t\320\264", "\b\f\n\r\t\320\264"}},

            "=== \b\f \r\t\320\264 ===\n\n"
            "\b \n\r\t\320\264"
            "\n"
            ,

            "  <p class=\"timestamp\"><b>\b\f \r\t\320\264</b></p>\n"
            "  <p class=\"text\">\n"
            "    \b <br>\n"
            "    \r\t\320\264\n"
            "  </p>\n"
            ,

            "[\n"
            "  {\n"
            "    \"timestamp\": \"\\b\\f \\r\\t\320\264\",\n"
            "    \"text\": \"\\b \\n\\r\\t\320\264\"\n"
            "  }\n"
            "]\n"},
    };

    const auto* historyFileName = "test_history_export.txt";

    for (const auto& test : tests) {
        dpso::HistoryUPtr history{dpsoHistoryOpen(historyFileName)};
        if (!history)
            test::fatalError(
                "dpsoHistoryOpen(\"{}\"): {}\n",
                historyFileName,
                dpsoGetError());

        if (!dpsoHistoryClear(history.get()))
            test::fatalError(
                "dpsoHistoryClear(): {}\n", dpsoGetError());

        for (const auto& entry : test.entries)
            if (!dpsoHistoryAppend(history.get(), &entry))
                test::fatalError(
                    "dpsoHistoryAppend(): {}\n", dpsoGetError());

        for (int i = 0; i < dpsoNumHistoryExportFormats; ++i) {
            const auto exportFormat =
                static_cast<DpsoHistoryExportFormat>(i);

            DpsoHistoryExportFormatInfo exportFormatInfo;
            dpsoHistoryGetExportFormatInfo(
                exportFormat, &exportFormatInfo);

            const auto exportedFileName =
                std::string{"test_history_export_"}
                + exportFormatInfo.name
                + exportFormatInfo.extensions[0];

            if (!dpsoHistoryExport(
                    history.get(),
                    exportedFileName.c_str(),
                    exportFormat))
                test::fatalError(
                    "dpsoHistoryExport(..., \"{}\", {})\n",
                    exportedFileName,
                    toStr(exportFormat));

            const auto& expectedData =
                test.exportedData[exportFormat];
            const auto gotData = test::utils::loadText(
                "testExport", exportedFileName.c_str());

            if (gotData != expectedData) {
                test::failure(
                    "testExport(): Unexpected exported {} data "
                    "for the \"{}\" case\n",
                    toStr(exportFormat),
                    test.description);
                test::utils::printFirstDifference(
                    expectedData.c_str(), gotData.c_str());
            }

            test::utils::removeFile(exportedFileName.c_str());
        }
    }

    test::utils::removeFile(historyFileName);
}


void testHistoryExport()
{
    testDetectExportFormat();
    testExport();
}


}


REGISTER_TEST(testHistoryExport);
