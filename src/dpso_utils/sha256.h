#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>


namespace dpso {


class Sha256 {
public:
    static constexpr std::size_t digestSize = 32;
    using Digest = std::array<std::uint8_t, digestSize>;

    void update(const void* data, std::size_t size);
    Digest getDigest() const;
private:
    class Context {
    public:
        Context();
        void update(const std::uint8_t* data, std::size_t size);
        Digest finalize();
    private:
        static constexpr auto stateSize = 8;
        static constexpr auto blockSize = 64;

        std::uint32_t state[stateSize];
        std::uint64_t numTransformedBytes;
        std::uint8_t buf[blockSize];
        std::uint32_t bufLen;

        void transform(const std::uint8_t block[blockSize]);
    };

    Context context;
};


std::string toHex(const Sha256::Digest& digest);


}
