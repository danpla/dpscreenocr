#include "sha256.h"

#include <algorithm>
#include <cassert>

#include "byte_order.h"


// See:
// * Wikipedia:
//     https://en.wikipedia.org/wiki/SHA-2
// * RFC 6234
//     https://datatracker.ietf.org/doc/html/rfc6234
// * FIPS PUB 180-4:
//     https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf


namespace dpso {
namespace {
namespace sha256 {


const std::uint32_t k[64]{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
    0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
    0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
    0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
    0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
    0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
    0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
    0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};


inline std::uint32_t rotR(std::uint32_t v, int n)
{
    return v >> n | v << (32 - n);
}


inline std::uint32_t ch(
    std::uint32_t x, std::uint32_t y, std::uint32_t z)
{
    return (x & y) ^ (~x & z);
}


inline std::uint32_t maj(
    std::uint32_t x, std::uint32_t y, std::uint32_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}


inline std::uint32_t bSig0(std::uint32_t x)
{
    return rotR(x, 2) ^ rotR(x, 13) ^ rotR(x, 22);
}


inline std::uint32_t bSig1(std::uint32_t x)
{
    return rotR(x, 6) ^ rotR(x, 11) ^ rotR(x, 25);
}


inline std::uint32_t sSig0(std::uint32_t x)
{
    return rotR(x, 7) ^ rotR(x, 18) ^ (x >> 3);
}


inline std::uint32_t sSig1(std::uint32_t x)
{
    return rotR(x, 17) ^ rotR(x, 19) ^ (x >> 10);
}


}
}


Sha256::Context::Context()
    : state{
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19}
    , numTransformedBytes{}
    , buf{}
    , bufLen{}
{
}


void Sha256::Context::update(
    const std::uint8_t* data, std::size_t size)
{
    if (bufLen > 0) {
        assert(bufLen < blockSize);
        const auto numCopy = std::min<std::size_t>(
            blockSize - bufLen, size);

        std::copy_n(data, numCopy, buf + bufLen);
        bufLen += numCopy;
        data += numCopy;
        size -= numCopy;

        if (bufLen == blockSize) {
            transform(buf);
            bufLen = 0;
        }
    }

    while (size >= blockSize) {
        transform(data);
        data += blockSize;
        size -= blockSize;
    }

    if (size > 0) {
        assert(bufLen == 0);
        std::copy_n(data, size, buf);
        bufLen = size;
    }
}


Sha256::Digest Sha256::Context::finalize()
{
    const auto bitSize = (numTransformedBytes + bufLen) * 8;

    assert(bufLen < blockSize);
    buf[bufLen++] = 0x80;

    const auto bitSizePos = blockSize - sizeof(bitSize);
    if (bufLen > bitSizePos) {
        std::fill(buf + bufLen, buf + blockSize, 0);
        transform(buf);
        bufLen = 0;
    }

    std::fill(buf + bufLen, buf + bitSizePos, 0);
    store<ByteOrder::big>(bitSize, buf + bitSizePos);
    transform(buf);

    Digest digest;
    for (auto i = 0; i < stateSize; ++i)
        store<ByteOrder::big>(
            state[i], digest.data() + sizeof(*state) * i);

    return digest;
}


void Sha256::Context::transform(const std::uint8_t block[blockSize])
{
    std::uint32_t w[64];

    for (auto i = 0; i < 16; ++i)
        load<ByteOrder::big>(w[i], block + sizeof(*w) * i);

    for (auto i = 16; i < 64; ++i)
        w[i] =
            sha256::sSig1(w[i - 2])
            + w[i - 7]
            + sha256::sSig0(w[i - 15])
            + w[i - 16];

    std::uint32_t ts[stateSize];
    std::copy_n(state, stateSize, ts);

    for (auto i = 0; i < 64; ++i) {
        const auto t1 =
            ts[7]
            + sha256::bSig1(ts[4])
            + sha256::ch(ts[4], ts[5], ts[6])
            + sha256::k[i]
            + w[i];

        const auto t2 =
            sha256::bSig0(ts[0]) + sha256::maj(ts[0], ts[1], ts[2]);

        std::copy_backward(ts, ts + stateSize - 1, ts + stateSize);
        ts[0] = t1 + t2;
        ts[4] += t1;
    }

    for (auto i = 0; i < stateSize; ++i)
        state[i] += ts[i];

    numTransformedBytes += blockSize;
}


void Sha256::update(const void* data, std::size_t size)
{
    context.update(static_cast<const std::uint8_t*>(data), size);
}


Sha256::Digest Sha256::getDigest() const
{
    return Sha256::Context{context}.finalize();
}


std::string toHex(const Sha256::Digest& digest)
{
    std::string result;
    result.reserve(digest.size() * 2);

    static const auto* hexChars = "0123456789abcdef";
    for (auto b : digest) {
        result += hexChars[b >> 4];
        result += hexChars[b & 0x0f];
    }

    return result;
}


}
