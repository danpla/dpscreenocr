
#include <cstring>
#include <iterator>
#include <vector>

#include "dpso_ext/history.h"
#include "dpso_utils/error_get.h"

#include "flow.h"
#include "utils.h"


namespace {


void cmpField(
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
    #define CMP(name) cmpField(#name, a.name, b.name, lineNum)

    CMP(timestamp);
    CMP(text);

    #undef CMP
}


#define CMP_ENTRIES(a, b) cmpEntries(a, b, __LINE__)


bool testCount(const DpsoHistory* history, int expected, int lineNum)
{
    const auto got = dpsoHistoryCount(history);
    if (got == expected)
        return true;

    test::failure(
        "line {}: dpsoHistoryCount(): Expected {}, got {}\n",
        lineNum,
        expected,
        got);
    return false;
}


#define TEST_COUNT(history, expected) \
    testCount(history, expected, __LINE__)


const auto* const historyFileName = "test_history.txt";


enum class IoTestMode {
    write,
    read
};


std::string toStr(IoTestMode mode)
{
    switch (mode) {
    case IoTestMode::write:
        return "IoTestMode::write";
    case IoTestMode::read:
        return "IoTestMode::read";
    }

    return {};
}


void testIo(IoTestMode mode)
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
            {"ts5   \f  a \f ", "text5 \n\n \n line  \n"}},
    };

    const auto numTests = std::size(tests);

    dpso::HistoryUPtr history{dpsoHistoryOpen(historyFileName)};
    if (!history) {
        test::utils::removeFile(historyFileName);
        test::fatalError(
            "testlIO({}): dpsoHistoryOpen(\"{}\"): {}\n",
            toStr(mode),
            historyFileName,
            dpsoGetError());
    }

    TEST_COUNT(
        history.get(), mode == IoTestMode::write ? 0 : numTests);

    for (int i = 0; i < static_cast<int>(numTests); ++i) {
        const auto& test = tests[i];

        if (mode == IoTestMode::write) {
            if (!dpsoHistoryAppend(history.get(), &test.inEntry)) {
                history.reset();
                test::utils::removeFile(historyFileName);

                test::fatalError(
                    "testIo(true): dpsoHistoryAppend(): {}\n",
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


void testTruncatedData()
{
    const struct Test {
        const char* description;
        const char* data;
        std::vector<DpsoHistoryEntry> expectedEntries;
    } tests[] = {
        {"No timestamp terminator (first entry)", "a", {}},
        {
            "No timestamp terminator (second entry)",
            "a\n\nb\f\nc",
            {{"a", "b"}}},
        {"Truncated timestamp terminator (first entry)", "a\n", {}},
        {
            "Truncated timestamp terminator (second entry)",
            "a\n\nb\f\nc\n", {{"a", "b"}}},
        {"Truncated entry separator", "a\n\nb\f", {{"a", "b"}}},
        {"Trailing entry separator", "a\n\nb\f\n", {{"a", "b"}}},
    };

    for (const auto& test : tests) {
        test::utils::saveText(
            "testTruncatedData", historyFileName, test.data);

        dpso::HistoryUPtr history{dpsoHistoryOpen(historyFileName)};
        if (!history) {
            test::failure(
                "testTruncatedData(): dpsoHistoryOpen() failed in "
                "the \"{}\" case: {}\n",
                test.description, dpsoGetError());
            continue;
        }

        if (!TEST_COUNT(history.get(), test.expectedEntries.size()))
            continue;

        for (int i = 0; i < dpsoHistoryCount(history.get()); ++i) {
            DpsoHistoryEntry entry;
            dpsoHistoryGet(history.get(), i, &entry);

            CMP_ENTRIES(entry, test.expectedEntries[i]);
        }

        if (DpsoHistoryEntry entry{"a", "b"};
                dpsoHistoryAppend(history.get(), &entry))
            test::failure(
                "testTruncatedData(): dpsoHistoryAppend() succeeded "
                "after loading truncated data\n");
    }

    test::utils::removeFile(historyFileName);
}


void testInvalidData()
{
    const struct Test {
        const char* description;
        const char* data;
    } tests[] = {
        {"Invalid timestamp terminator", "a\nb"},
        {"Invalid entry separator", "a\n\nb\f*a\n\nb"},
    };

    for (const auto& test : tests) {
        test::utils::saveText(
            "testInvalidData", historyFileName, test.data);

        dpso::HistoryUPtr history{dpsoHistoryOpen(historyFileName)};
        if (!history)
            continue;

        test::failure(
            "testInvalidData(): dpsoHistoryOpen() doesn't fail in "
            "the \"{}\" case\n",
            test.description);
    }

    test::utils::removeFile(historyFileName);
}


void testHistory()
{
    testIo(IoTestMode::write);
    testIo(IoTestMode::read);
    testTruncatedData();
    testInvalidData();
}


}


REGISTER_TEST(testHistory);
