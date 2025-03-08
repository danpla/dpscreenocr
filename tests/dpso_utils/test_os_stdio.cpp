#include <cerrno>
#include <string>
#include <vector>

#include "dpso_utils/os.h"
#include "dpso_utils/os_stdio.h"

#include "flow.h"
#include "utils.h"


using namespace dpso;


namespace {


const auto* const testUnicodeFileName =
    // 汉语.txt
    "\346\261\211\350\257\255.txt";


void testFopen()
{
    os::StdFileUPtr fp{os::fopen(testUnicodeFileName, "wb")};
    if (!fp) {
        test::failure(
            "os::fopen(\"{}\"): {}",
            testUnicodeFileName, os::getErrnoMsg(errno));
        return;
    }

    fp.reset();
    test::utils::removeFile(testUnicodeFileName);
}


void testSyncFile()
{
    const auto* fileName = "test_sync_file.txt";

    os::StdFileUPtr fp{os::fopen(fileName, "wb")};
    if (!fp)
        test::fatalError(
            "testSyncFile: os::fopen(\"{}\"): {}",
            fileName, os::getErrnoMsg(errno));

    try {
        os::syncFile(fp.get());
    } catch (os::Error& e) {
        test::failure("os::syncFile(): {}", e.what());
    }

    fp.reset();
    test::utils::removeFile(fileName);
}


void testOsStdio()
{
    testFopen();
    testSyncFile();
}


}


REGISTER_TEST(testOsStdio);
