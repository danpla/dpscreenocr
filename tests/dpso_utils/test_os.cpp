#include <cerrno>
#include <string>
#include <vector>

#include "dpso_utils/os.h"
#include "dpso_utils/scope_exit.h"

#include "flow.h"
#include "utils.h"


// We only test those functions that are not implemented on top of
// std::filesystem.


using namespace dpso;


namespace {


template<typename Fn>
bool checkFileNotFoundError(Fn fn, const char* fnName)
{
    try {
        fn();
        test::failure(
            "{}() for a nonexistent file didn't threw an error",
            fnName);
    } catch (os::FileNotFoundError&) {
        return true;
    } catch (os::Error& e) {
        test::failure(
            "{}() for a nonexistent file threw an error "
            "(\"{}\") of class other than FileNotFoundError",
            fnName, e.what());
    }

    return false;
}


#define CHECK_FILE_NOT_FOUND_ERROR(FN, ...) \
    checkFileNotFoundError([]{ FN(__VA_ARGS__); }, #FN)


void testSyncDir()
{
    const auto* dirPath = ".";

    try {
        os::syncDir(dirPath);
    } catch (os::Error& e) {
        test::failure("os::syncDir(\"{}\"): {}", dirPath, e.what());
    }
}


void testLoadData()
{
    const std::string filePath{"test_load_data.txt"};

    test::utils::saveText(
        "testLoadData", filePath.c_str(), filePath.c_str());

    try {
        const auto data = os::loadData(filePath.c_str());
        if (data != filePath)
            test::failure(
                "os::loadData(\"{}\"): expected \"{}\", got \"{}\"",
                filePath, data, filePath);
    } catch (os::Error& e) {
        test::failure("os::loadData(\"{}\"): {}", filePath, e.what());
    }

    test::utils::removeFile(filePath.c_str());

    CHECK_FILE_NOT_FOUND_ERROR(os::loadData, "nonexistent_file");
}


void testOs()
{
    testSyncDir();
    testLoadData();
}


}


REGISTER_TEST(testOs);
