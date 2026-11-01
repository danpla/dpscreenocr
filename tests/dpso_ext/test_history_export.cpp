#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "dpso_ext/history.h"
#include "dpso_ext/history_export.h"
#include "dpso_utils/error_get.h"

#include "flow.h"
#include "utils.h"


namespace {


std::string toStr(DpsoHistoryExportFormat exportFormat)
{
    DpsoHistoryExportFormatInfo exportFormatInfo;
    dpsoHistoryGetExportFormatInfo(exportFormat, &exportFormatInfo);
    return exportFormatInfo.name;
}


void testDetectExportFormat()
{
    const struct {
        std::vector<std::string> extensions;
        DpsoHistoryExportFormat exportFormat;
    } tests[]{
        {
            {".txt", ".Txt", ".TXT"},
            dpsoHistoryExportFormatPlainText
        },
        {
            {".html", ".Html", ".HTML", ".htm", ".Htm", ".HTM"},
            dpsoHistoryExportFormatHtml
        },
        {
            {".json", ".Json", ".JSON"},
            dpsoHistoryExportFormatJson
        },
    };
    static_assert(std::size(tests) == dpsoNumHistoryExportFormats);

    for (const std::string prefix : {"file", "dir.name/file"})
        for (const auto& test : tests) {
            for (const auto& ext : test.extensions) {
                const auto path = prefix + ext;

                DpsoHistoryExportFormat exportFormat{};
                if (!dpsoHistoryDetectExportFormat(
                        path.c_str(), &exportFormat)) {
                    test::failure(
                        "dpsoHistoryDetectExportFormat(\"{}\", ...) "
                        "returned false",
                        path);
                    continue;
                }

                if (exportFormat != test.exportFormat) {
                    test::failure(
                        "dpsoHistoryDetectExportFormat(\"{}\", ...): "
                        "expected {}, got {}",
                        path,
                        toStr(test.exportFormat),
                        toStr(exportFormat));
                    continue;
                }

                if (!dpsoHistoryDetectExportFormat(
                        path.c_str(), nullptr))
                    test::failure(
                        "dpsoHistoryDetectExportFormat(\"{}\", "
                        "nullptr) returned false",
                        path);
            }

            const auto path = prefix + ".unknown_ext";

            DpsoHistoryExportFormat exportFormat{};
            if (dpsoHistoryDetectExportFormat(
                    path.c_str(), &exportFormat)) {
                test::failure(
                    "dpsoHistoryDetectExportFormat(\"{}\", ...) "
                    "returned true (detected {})",
                    path, toStr(exportFormat));
                continue;
            }

            if (dpsoHistoryDetectExportFormat(path.c_str(), nullptr))
                test::failure(
                    "dpsoHistoryDetectExportFormat(\"{}\", nullptr) "
                    "returned true",
                    path);
        }
}


void testExport()
{
    static const std::string_view htmlBegin{
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <meta name=\"viewport\" "
            "content=\"width=device-width, initial-scale=1\">\n"
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
        "<body>\n"};

    static const std::string_view htmlEnd{
        "</body>\n"
        "</html>\n"};

    const struct Test {
        std::string_view description;
        std::vector<DpsoHistoryEntry> entries;
        std::vector<std::string> exportedData;

        Test(
                std::string_view description,
                std::vector<DpsoHistoryEntry>&& entries,
                std::string_view txtData,
                std::string_view htmlBodyData,
                std::string_view jsonData)
            : description{description}
            , entries{std::move(entries)}
            , exportedData{
                test::utils::lfToNativeNewline(txtData),
                test::utils::lfToNativeNewline(htmlBegin)
                    + test::utils::lfToNativeNewline(htmlBodyData)
                    + test::utils::lfToNativeNewline(htmlEnd),
                test::utils::lfToNativeNewline(jsonData)}
        {
            if (exportedData.size() != dpsoNumHistoryExportFormats)
                test::fatalError(
                    "Unexpected number of exportedData entries. "
                    "Seems like some export formats were added or "
                    "removed, so exportedData should be adjusted.");
        }
    } tests[]{
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

            "  <p class=\"timestamp\"></p>\n"
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

            "  <p class=\"timestamp\"></p>\n"
            "  <p class=\"text\">\n"
            "\n"
            "  </p>\n"
            "  <hr>\n"
            "  <p class=\"timestamp\"></p>\n"
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

            "  <p class=\"timestamp\">ts</p>\n"
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

            "  <p class=\"timestamp\">ts 1</p>\n"
            "  <p class=\"text\">\n"
            "    text 1\n"
            "  </p>\n"
            "  <hr>\n"
            "  <p class=\"timestamp\">ts 2</p>\n"
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

            "  <p class=\"timestamp\">ts line 1 ts line 2</p>\n"
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

            "  <p class=\"timestamp\">ts</p>\n"
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

            "  <p class=\"timestamp\">'\"&lt;&gt;&amp;</p>\n"
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

            "  <p class=\"timestamp\">\b\f \r\t\320\264</p>\n"
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
                "dpsoHistoryOpen(\"{}\"): {}",
                historyFileName, dpsoGetError());

        if (!dpsoHistoryClear(history.get()))
            test::fatalError(
                "dpsoHistoryClear(): {}", dpsoGetError());

        for (const auto& entry : test.entries)
            if (!dpsoHistoryAppend(history.get(), &entry))
                test::fatalError(
                    "dpsoHistoryAppend(): {}", dpsoGetError());

        for (int i{}; i < dpsoNumHistoryExportFormats; ++i) {
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
                    "dpsoHistoryExport(..., \"{}\", {})",
                    exportedFileName, toStr(exportFormat));

            const auto& expectedData =
                test.exportedData[exportFormat];
            const auto gotData = test::utils::loadText(
                "testExport", exportedFileName);

            if (gotData != expectedData) {
                test::failure(
                    "testExport(): Unexpected exported {} data "
                    "for the \"{}\" case",
                    toStr(exportFormat),
                    test.description);
                test::utils::printFirstDifference(
                    expectedData, gotData);
            }

            test::utils::removeFile(exportedFileName);
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
