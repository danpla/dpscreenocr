
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
        "line {}: DpsoHistoryEntry::{}: {} != {}",
        lineNum,
        name,
        test::utils::toStr(a),
        test::utils::toStr(b));
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
        "line {}: dpsoHistoryCount(): Expected {}, got {}",
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

        Test(const DpsoHistoryEntry& entry)
            : Test{entry, entry}
        {}

        Test(
                const DpsoHistoryEntry& inEntry,
                const DpsoHistoryEntry& outEntry)
            : inEntry{inEntry}
            , outEntry{outEntry}
        {}
    } tests[]{
        {{"ts1", "text1 \n\t\r line \n\t\r line \n\t\r "}},
        {{"", ""}},
        {{"", "text2"}},
        {{"ts3", ""}},
        {{"ts4", "text4"}},
        {
            {"ts5 \n\n\f\n a \f\n", "text5 \n\n\f\n line \f\n"},
            {"ts5   \f  a \f ", "text5 \n\n \n line  \n"}},
    };

    const auto numTests = std::size(tests);

    dpso::HistoryUPtr history{dpsoHistoryOpen(historyFileName)};
    if (!history) {
        test::utils::removeFile(historyFileName);
        test::fatalError(
            "testlIO({}): dpsoHistoryOpen(\"{}\"): {}",
            mode, historyFileName, dpsoGetError());
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
                    "testIo(true): dpsoHistoryAppend(): {}",
                    dpsoGetError());
            }

            TEST_COUNT(history.get(), i + 1);
        }

        DpsoHistoryEntry outEntry;
        dpsoHistoryGet(history.get(), i, &outEntry);
        CMP_ENTRIES(outEntry, test.outEntry);
    }
}


void testTruncatedData()
{
    const DpsoHistoryEntry extraEntry{"extraTs", "extraText"};

    const struct {
        const char* description;
        const char* data;
        std::vector<DpsoHistoryEntry> expectedEntries;
        const char* finalData;  // After appending extraEntry.
    } tests[]{
        {
            "No timestamp terminator (first entry)",
            "ts1",
            {},
            "extraTs\n\nextraText"},
        {
            "No timestamp terminator (second entry)",
            "ts1\n\ntext1\f\nts2",
            {{"ts1", "text1"}},
            "ts1\n\ntext1\f\nextraTs\n\nextraText"},
        {
            "Truncated timestamp terminator (first entry)",
            "ts1\n",
            {},
            "extraTs\n\nextraText"},
        {
            "Truncated timestamp terminator (second entry)",
            "ts1\n\ntext1\f\nts2\n",
            {{"ts1", "text1"}},
            "ts1\n\ntext1\f\nextraTs\n\nextraText"},
        {
            "Truncated entry separator",
            "ts1\n\ntext1\f",
            {{"ts1", "text1"}},
            "ts1\n\ntext1\f\nextraTs\n\nextraText"},
        {
            "Trailing entry separator",
            "ts1\n\ntext1\f\n",
            {{"ts1", "text1"}},
            "ts1\n\ntext1\f\nextraTs\n\nextraText"},
    };

    for (const auto& test : tests) {
        test::utils::saveText(
            "testTruncatedData", historyFileName, test.data);

        dpso::HistoryUPtr history{dpsoHistoryOpen(historyFileName)};
        if (!history) {
            test::failure(
                "testTruncatedData(): dpsoHistoryOpen() failed in "
                "the \"{}\" case: {}",
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

        if (!dpsoHistoryAppend(history.get(), &extraEntry)) {
            test::failure(
                "testTruncatedData(): dpsoHistoryAppend() failed "
                "after loading truncated data");
            continue;
        }

        history.reset();

        const auto finalData = test::utils::loadText(
            "testTruncatedData", historyFileName);
        if (finalData == test.finalData)
            continue;

        test::failure(
            "testTruncatedData(): Unexpected final data");
        test::utils::printFirstDifference(
            test.finalData, finalData.c_str());
    }

    test::utils::removeFile(historyFileName);
}


void testInvalidData()
{
    const struct {
        const char* description;
        const char* data;
    } tests[]{
        {"Invalid timestamp terminator", "ts1\nb"},
        {"Invalid entry separator", "ts1\n\ntext1\f*ts2\n\ntext2"},
    };

    for (const auto& test : tests) {
        test::utils::saveText(
            "testInvalidData", historyFileName, test.data);

        dpso::HistoryUPtr history{dpsoHistoryOpen(historyFileName)};
        if (!history)
            continue;

        test::failure(
            "testInvalidData(): dpsoHistoryOpen() doesn't fail in "
            "the \"{}\" case",
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
