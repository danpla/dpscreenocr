
#include <string>

#include "dpso_utils/sha256.h"

#include "flow.h"


// Test vectors are from:
// https://www.di-mgt.com.au/sha_testvectors.html
// https://datatracker.ietf.org/doc/html/rfc6234
// https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHA2_Additional.pdf


static void testSha256()
{
    const struct Test {
        std::string str;
        const char* digest;
        int numRepeats;
    } tests[] = {
        {
            "abc",
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61"
                "f20015ad",
            1},
        {
            "",
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b"
                "7852b855",
            1},
        {
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnop"
                "q",
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd4"
                "19db06c1",
            1},
        {
            "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
                "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopq"
                "rstu",
            "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac4503"
                "7afee9d1",
            1},
        {
            "a",
            "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39cc"
                "c7112cd0",
            1000000},
        {
            "01234567012345670123456701234567012345670123456701234567"
            "01234567",
            "594847328451bdfa85056225462cc1d867d877fb388df0ce35f25ab5"
                "562bfbb5",
            10},
        {
            {"\0", 1},
            "d4817aa5497628e7c77e6b606107042bbba3130888c5f47a375e6179"
                "be789fbb",
            56},
        {
            {"\0", 1},
            "65a16cb7861335d5ace3c60718b5052e44660726da4cd13bb745381b"
                "235a1785",
            57},
        {
            {"\0", 1},
            "f5a5fd42d16a20302798ef6ed309979b43003d2320d9f0e8ea9831a9"
                "2759fb4b",
            64},
    };

    for (const auto& test : tests) {
        dpso::Sha256 h;

        for (auto i = 0; i < test.numRepeats; ++i)
            h.update(test.str.data(), test.str.size());

        if (dpso::toHex(h.getDigest()) == test.digest)
            continue;

        test::failure(
            "SHA-256 failed for \"{}\" (repeats: {})\n{}\n{}\n",
            test.str,
            test.numRepeats,
            test.digest,
            dpso::toHex(h.getDigest()));
        break;
    }
}


REGISTER_TEST(testSha256);
