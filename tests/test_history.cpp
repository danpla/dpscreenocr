
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "dpso/error.h"
#include "dpso_utils/history.h"

#include "flow.h"
#include "utils.h"


static void cmpFields(
    const char* name, const char* a, const char* b, int line)
{
    if (std::strcmp(a, b) == 0)
        return;

    std::fprintf(
        stderr,
        "line %i: DpsoHistoryEntry::%s don't match: "
        "\"%s\" != \"%s\"\n",
        line,
        name,
        test::utils::escapeStr(a).c_str(),
        test::utils::escapeStr(b).c_str());
    test::failure();
}


static void cmpEntries(
    const DpsoHistoryEntry& a, const DpsoHistoryEntry& b, int line)
{
    #define CMP(name) cmpFields(#name, a.name, b.name, line)

    CMP(timestamp);
    CMP(text);

    #undef CMP
}


#define CMP_ENTRIES(a, b) cmpEntries(a, b, __LINE__)


static void testCount(
    const DpsoHistory* history, int expected, int line)
{
    const auto got = dpsoHistoryCount(history);
    if (got == expected)
        return;

    std::fprintf(
        stderr,
        "line %i: dpsoHistoryCount(): Expected %i, got %i\n",
        line,
        expected,
        got);
    test::failure();
}


#define TEST_COUNT(history, expected) \
    testCount(history, expected, __LINE__)


const char* const historyFileName = "test_history.txt";


static void testIO(bool append)
{
    struct Test {
        DpsoHistoryEntry inEntry;
        DpsoHistoryEntry outEntry;
    };

    const Test tests[] = {
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

    static const auto numTests = sizeof(tests) / sizeof(*tests);

    dpso::HistoryUPtr history{dpsoHistoryOpen(historyFileName)};
    if (!history) {
        std::fprintf(
            stderr,
            "testlIO(%sappend): "
            "dpsoHistoryOpen(\"%s\") failed: %s\n",
            append ? "" : "!",
            historyFileName,
            dpsoGetError());
        std::remove(historyFileName);
        std::exit(EXIT_FAILURE);
    }

    TEST_COUNT(history.get(), append ? 0 : numTests);

    for (int i = 0; i < static_cast<int>(numTests); ++i) {
        const auto& test = tests[i];

        if (append) {
            if (!dpsoHistoryAppend(history.get(), &test.inEntry)) {
                std::fprintf(
                    stderr,
                    "testIO(%sappend): "
                    "dpsoHistoryAppend() failed: %s\n",
                    append ? "" : "!",
                    dpsoGetError());
                history.reset();
                std::remove(historyFileName);
                std::exit(EXIT_FAILURE);
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
    struct Test {
        const char* description;
        const char* data;
    };

    const Test tests[] = {
        {"No timestamp terminator", "a"},
        {"Invalid timestamp terminator", "a\nb"},
        {"Truncated entry separator", "a\n\nb\f"},
        {"Invalid entry separator", "a\n\nb\f*a\n\nb\f\n"},
        {"Trailing entry separator", "a\n\nb\f\n"},
    };

    for (const auto& test : tests) {
        auto* fp = std::fopen(historyFileName, "wb");
        if (!fp) {
            std::fprintf(
                stderr,
                "loadInvalidData(): "
                "fopen(\"%s\", \"wb\") failed: %s\n",
                historyFileName,
                std::strerror(errno));
            std::exit(EXIT_FAILURE);
        }

        std::fputs(test.data, fp);
        std::fclose(fp);

        dpso::HistoryUPtr history{dpsoHistoryOpen(historyFileName)};
        const auto opened = history != nullptr;
        history.reset();
        std::remove(historyFileName);

        if (!opened)
            continue;

        std::fprintf(
            stderr,
            "loadInvalidData(): dpsoHistoryOpen() doesn't fail in "
            "\"%s\" case\n",
            test.description);
        test::failure();
    }
}


static void testHistory()
{
    testIO(true);
    testIO(false);
    testInvalidData();
}


REGISTER_TEST(testHistory);
