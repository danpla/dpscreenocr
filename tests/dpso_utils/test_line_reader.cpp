
#include <optional>
#include <vector>

#include "dpso_utils/file.h"
#include "dpso_utils/line_reader.h"
#include "dpso_utils/os.h"

#include "flow.h"
#include "utils.h"


namespace {


void testLineReader()
{
    const struct Test {
        const char* text;
        std::vector<std::string> expectedLines;
    } tests[] = {
        {"", {}},
        {" ", {" "}},
        {"\r", {""}},
        {"\n", {""}},
        {"\r\n", {""}},
        {"\n\r", {"", ""}},
        {"\r\r", {"", ""}},
        {"\n\n", {"", ""}},
        {"a", {"a"}},
        {"a\n", {"a"}},
        {"a\nb\r", {"a", "b"}},
        {"a\nb\rc", {"a", "b", "c"}},
        {"a\nb\rc\r\n", {"a", "b", "c"}},
    };

    const auto* fileName = "test_read_line.txt";

    for (const auto& test : tests) {
        test::utils::saveText(
            "testReadLine", fileName, test.text);

        std::optional<dpso::File> file;
        try {
            file.emplace(fileName, dpso::File::Mode::read);
        } catch (dpso::os::Error& e) {
            test::fatalError(
                "File(\"{}\", Mode::read): {}\n", fileName, e.what());
        }

        std::string line{"initial line content"};
        std::vector<std::string> lines;

        dpso::LineReader lineReader{*file};
        try {
            while (lineReader.readLine(line))
                lines.push_back(line);
        } catch (dpso::os::Error& e) {
            test::fatalError(
                "LineReader::readLine(): {}\n", e.what());
        }

        if (!line.empty())
            test::failure(
                "LineReader::readLine() didn't cleared the line "
                "after finishing reading\n");

        line = "content for an extra LineReader::readLine()";
        if (lineReader.readLine(line))
            test::failure(
                "An extra LineReader::readLine() returned true after "
                "reading was finished\n");

        if (!line.empty())
            test::failure(
                "An extra LineReader::readLine() didn't cleared the "
                "line\n");

        if (lines != test.expectedLines)
            test::failure(
                "Unexpected lines from LineReader::readLine() for "
                "\"{}\": expected {}, got {}\n",
                test::utils::escapeStr(test.text),
                test::utils::toStr(
                    test.expectedLines.begin(),
                    test.expectedLines.end()),
                test::utils::toStr(lines.begin(), lines.end()));
    }

    test::utils::removeFile(fileName);
}


}


REGISTER_TEST(testLineReader);
