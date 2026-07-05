#pragma once

#include <filesystem>

#include "dp_intl/data_stream.h"


namespace dp::intl {


/**
 * DataStream for a file.
 */
class FileDataStream : public DataStream {
public:
    /**
     * Open a file.
     *
     * \throws FileNotFoundError if the file does not exist, or Error
     *     on other errors.
     */
    explicit FileDataStream(const std::filesystem::path& path);
    ~FileDataStream();

    FileDataStream(const FileDataStream&) = delete;
    FileDataStream& operator=(const FileDataStream&) = delete;

    FileDataStream(FileDataStream&&) = delete;
    FileDataStream& operator=(FileDataStream&&) = delete;

    std::size_t readSome(void* dst, std::size_t dstSize) override;
    void setPosition(std::int64_t newPosition) override;
private:
    void* impl;
};


}
