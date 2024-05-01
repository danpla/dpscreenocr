
#include <string>

#include "dpso_utils/sha256_file.h"

#include "flow.h"
#include "utils.h"


namespace {


void testCalcFileSha256()
{
    const auto* testFileName = "test_calc_file_sha256.txt";
    const auto* data =
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    const auto* digest =
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd4"
        "19db06c1";

    test::utils::saveText("testCalcFileSha256", testFileName, data);

    std::string calculatedDigest;

    try {
        calculatedDigest = dpso::calcFileSha256(testFileName);
    } catch (dpso::Sha256FileError& e) {
        test::failure(
            "calcFileSha256(\"{}\"): {}", testFileName, e.what());

        test::utils::removeFile(testFileName);
        return;
    }

    test::utils::removeFile(testFileName);

    if (calculatedDigest == digest)
        return;

    test::failure("calcFileSha256(): Wrong digest");
    test::utils::printFirstDifference(
        digest, calculatedDigest.c_str());
}


void testSaveSha256File()
{
    const std::string digest =
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61"
        "f20015ad";
    const std::string testFileName = "test_sha256_save.txt";
    const auto testSha256FileName =
        testFileName + dpso::sha256FileExt;

    try {
        dpso::saveSha256File(
            testFileName.c_str(), digest.c_str());
    } catch (dpso::Sha256FileError& e) {
        test::failure(
            "saveSha256File(\"{}\", ...): {}",
            testFileName, e.what());
        return;
    }

    const auto gotData = test::utils::loadText(
        "testSaveSha256File", testSha256FileName.c_str());

    test::utils::removeFile(testSha256FileName.c_str());

    const auto expectedData = digest + " *" + testFileName + "\n";

    if (gotData == expectedData)
        return;

    test::failure("saveSha256File(): Unexpected content");
    test::utils::printFirstDifference(
        expectedData.c_str(), gotData.c_str());
}


void testLoadNonexistentSha256File()
{
    std::string digest;
    try {
        digest = dpso::loadSha256File("nonexistent");
    } catch (dpso::Sha256FileError& e) {
        test::failure(
            "loadSha256File() threw an exception for a nonexistent "
            "file: {}",
            e.what());
        return;
    }

    if (!digest.empty())
        test::failure(
            "loadSha256File() returned a non-empty digest \"{}\" for "
            "a nonexistent file",
            digest.c_str());
}


void testLoadValidSha256File()
{
    const std::string digest =
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61"
        "f20015ad";
    const std::string testFileName = "test_sha256_load.txt";
    const auto testSha256FileName =
        testFileName + dpso::sha256FileExt;

    const struct Test {
        const char* description;
        std::string data;
    } tests[]{
        {
            "Normal",
            digest + " *" + testFileName + "\n"},
        {
            "CRLF",
            digest + " *" + testFileName + "\r\n"},
        {
            "CR only",
            digest + " *" + testFileName + "\r"},
        {
            "No trailing newline",
            digest + " *" + testFileName},
    };

    for (const auto& test : tests) {
        test::utils::saveText(
            "testLoadValidSha256File",
            testSha256FileName.c_str(),
            test.data.c_str());

        std::string loadedDigest;
        try {
            loadedDigest = dpso::loadSha256File(
                testFileName.c_str());
        } catch (dpso::Sha256FileError& e) {
            test::failure(
                "loadSha256File() threw an exception for the \"{}\" "
                "case: {}",
                test.description, e.what());
            continue;
        }

        if (loadedDigest == digest)
            continue;

        test::failure(
            "loadSha256File() returned an invalid digest");
        test::utils::printFirstDifference(
            digest.c_str(), loadedDigest.c_str());
    }

    test::utils::removeFile(testSha256FileName.c_str());
}


void testLoadInvalidSha256File()
{
    const std::string digest =
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61"
        "f20015ad";
    const std::string testFileName = "test_sha256_load.txt";
    const auto testSha256FileName =
        testFileName + dpso::sha256FileExt;

    const struct Test {
        const char* description;
        std::string data;
    } tests[]{
        {"Leading spaces", " " + digest + " *" + testFileName},
        {"Trailing spaces", digest + " *" + testFileName + " "},
        {"Extra digest separators", digest + "   *" + testFileName},
        {"Text digest mode", digest + "  " + testFileName},
        {"Invalid digest mode", digest + " |" + testFileName},
        {"No digest", " *" + testFileName},
        {"Only digest", digest},
        {"No file name", digest + " *" },
        {
            "Truncated digest",
            digest.substr(digest.size() - 2) + " *" + testFileName},
        {"Overlong digest", digest + "ab *" + testFileName},
        {"Unexpected file name", digest + " *" + "unexpected"},
        {
            "Two digest lines",
            digest + " *" + testFileName + "\n"
            + digest + " *" + testFileName},
        {
            "Two trailing line feeds",
            digest + " *" + testFileName + "\n\n"},
    };

    for (const auto& test : tests) {
        test::utils::saveText(
            "testLoadInvalidSha256File",
            testSha256FileName.c_str(),
            test.data.c_str());

        try {
            dpso::loadSha256File(testFileName.c_str());
        } catch (dpso::Sha256FileError&) {
            continue;
        }

        test::failure(
            "loadSha256File() didn't failed for the \"{}\" case",
            test.description);
    }

    test::utils::removeFile(testSha256FileName.c_str());
}


void testSha256File()
{
    testCalcFileSha256();

    testSaveSha256File();

    testLoadNonexistentSha256File();
    testLoadValidSha256File();
    testLoadInvalidSha256File();
}


}


REGISTER_TEST(testSha256File);
