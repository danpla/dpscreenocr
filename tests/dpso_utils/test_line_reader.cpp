
#include <optional>
#include <vector>

#include "dpso_utils/line_reader.h"
#include "dpso_utils/os.h"
#include "dpso_utils/stream/file_stream.h"

#include "flow.h"
#include "utils.h"


namespace {


void testLineReader()
{
    const struct {
        const char* text;
        std::vector<std::string> expectedLines;
    } tests[]{
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

        std::optional<dpso::FileStream> file;
        try {
            file.emplace(fileName, dpso::FileStream::Mode::read);
        } catch (dpso::os::Error& e) {
            test::fatalError(
                "FileStream(\"{}\", Mode::read): {}",
                fileName, e.what());
        }

        std::string line{"initial line content"};
        std::vector<std::string> lines;

        dpso::LineReader lineReader{*file};
        try {
            while (lineReader.readLine(line))
                lines.push_back(line);
        } catch (dpso::StreamError& e) {
            test::fatalError("LineReader::readLine(): {}", e.what());
        }

        if (!line.empty())
            test::failure(
                "LineReader::readLine() didn't cleared the line "
                "after finishing reading");

        line = "content for an extra LineReader::readLine()";
        if (lineReader.readLine(line))
            test::failure(
                "An extra LineReader::readLine() returned true after "
                "reading was finished");

        if (!line.empty())
            test::failure(
                "An extra LineReader::readLine() didn't cleared the "
                "line");

        if (lines != test.expectedLines)
            test::failure(
                "Unexpected lines from LineReader::readLine() for "
                "{}: expected {}, got {}",
                test::utils::toStr(test.text),
                test::utils::toStr(test.expectedLines),
                test::utils::toStr(lines));
    }

    test::utils::removeFile(fileName);
}


}


REGISTER_TEST(testLineReader);
