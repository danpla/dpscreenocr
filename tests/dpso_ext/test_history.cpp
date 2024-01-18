
#include <cstring>
#include <iterator>

#include "dpso_ext/history.h"
#include "dpso_utils/error_get.h"

#include "flow.h"
#include "utils.h"


namespace {


void cmpFields(
    const char* name, const char* a, const char* b, int lineNum)
{
    if (std::strcmp(a, b) == 0)
        return;

    test::failure(
        "line {}: DpsoHistoryEntry::{}: \"{}\" != \"{}\"\n",
        lineNum,
        name,
        test::utils::escapeStr(a),
        test::utils::escapeStr(b));
}


void cmpEntries(
    const DpsoHistoryEntry& a, const DpsoHistoryEntry& b, int lineNum)
{
    #define CMP(name) cmpFields(#name, a.name, b.name, lineNum)

    CMP(timestamp);
    CMP(text);

    #undef CMP
}


#define CMP_ENTRIES(a, b) cmpEntries(a, b, __LINE__)


void testCount(const DpsoHistory* history, int expected, int lineNum)
{
    const auto got = dpsoHistoryCount(history);
    if (got == expected)
        return;

    test::failure(
        "line {}: dpsoHistoryCount(): Expected {}, got {}\n",
        lineNum,
        expected,
        got);
}


#define TEST_COUNT(history, expected) \
    testCount(history, expected, __LINE__)


const auto* const historyFileName = "test_history.txt";


void testIO(bool append)
{
    const struct Test {
        DpsoHistoryEntry inEntry;
        DpsoHistoryEntry outEntry;
    } tests[] = {
        {{"ts1", "text1 \n\t\r line \n\t\r line \n\t\r "}, {}},
        {{"", ""}, {}},
        {{"", "text2"}, {}},
        {{"ts3", ""}, {}},
        {{"ts4", "text4"}, {}},
        {
            {"ts5 \n\n\f\n a \f\n", "text5 \n\n\f\n line \f\n"},
            {"ts5   \f  a \f ", "text5 \n\n \n line  \n"},
        },
    };

    const auto numTests = std::size(tests);

    dpso::HistoryUPtr history{dpsoHistoryOpen(historyFileName)};
    if (!history) {
        test::utils::removeFile(historyFileName);
        test::fatalError(
            "testlIO({}append): dpsoHistoryOpen(\"{}\"): {}\n",
            append ? "" : "!",
            historyFileName,
            dpsoGetError());
    }

    TEST_COUNT(history.get(), append ? 0 : numTests);

    for (int i = 0; i < static_cast<int>(numTests); ++i) {
        const auto& test = tests[i];

        if (append) {
            if (!dpsoHistoryAppend(history.get(), &test.inEntry)) {
                history.reset();
                test::utils::removeFile(historyFileName);

                test::fatalError(
                    "testIO(true): dpsoHistoryAppend(): {}\n",
                    dpsoGetError());
            }

            TEST_COUNT(history.get(), i + 1);
        }

        auto expectedOutEntry = test.outEntry;
        if (!expectedOutEntry.text)
            expectedOutEntry.text = test.inEntry.text;
        if (!expectedOutEntry.timestamp)
            expectedOutEntry.timestamp = test.inEntry.timestamp;

        DpsoHistoryEntry outEntry;
        dpsoHistoryGet(history.get(), i, &outEntry);
        CMP_ENTRIES(outEntry, expectedOutEntry);
    }
}


void testInvalidData()
{
    const struct Test {
        const char* description;
        const char* data;
    } tests[] = {
        {"No timestamp terminator", "a"},
        {"Invalid timestamp terminator", "a\nb"},
        {"Truncated entry separator", "a\n\nb\f"},
        {"Invalid entry separator", "a\n\nb\f*a\n\nb\f\n"},
        {"Trailing entry separator", "a\n\nb\f\n"},
    };

    for (const auto& test : tests) {
        test::utils::saveText(
            "testInvalidData", historyFileName, test.data);

        dpso::HistoryUPtr history{dpsoHistoryOpen(historyFileName)};
        const auto opened = history != nullptr;
        history.reset();
        test::utils::removeFile(historyFileName);

        if (!opened)
            continue;

        test::failure(
            "testInvalidData(): dpsoHistoryOpen() doesn't fail in "
            "the \"{}\" case\n",
            test.description);
    }
}


void testHistory()
{
    testIO(true);
    testIO(false);
    testInvalidData();
}


}


REGISTER_TEST(testHistory);
