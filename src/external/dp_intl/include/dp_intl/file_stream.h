#pragma once

#include <filesystem>

#include "dp_intl/impl/export.h"
#include "dp_intl/stream.h"


namespace dp::intl {


/**
 * File stream.
 */
class DP_INTL_EXPORT FileStream : public Stream {
public:
    /**
     * Open a file.
     *
     * \throws FileNotFoundError if the file does not exist, or
     *     dp::intl::Error on other errors.
     */
    explicit FileStream(const std::filesystem::path& path);
    ~FileStream();

    FileStream(const FileStream&) = delete;
    FileStream& operator=(const FileStream&) = delete;

    FileStream(FileStream&&) = delete;
    FileStream& operator=(FileStream&&) = delete;

    std::size_t readSome(void* dst, std::size_t dstSize) override;
    void setPosition(std::int64_t newPosition) override;
private:
    void* impl;
};


}
