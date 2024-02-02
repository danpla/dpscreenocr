
#pragma once

#include <functional>
#include <memory>
#include <stdexcept>


namespace dpso::ocr {


class DataLock {
public:
    class DataLockedError : public std::runtime_error {
        using runtime_error::runtime_error;
    };

    DataLock();

    // Throws DataLockedError if data is already locked by another
    // DataLock.
    DataLock(const char* engineId, const char* dataDir);

    ~DataLock();

    DataLock(const DataLock&) = delete;
    DataLock& operator=(const DataLock&) = delete;

    DataLock(DataLock&& other) noexcept;
    DataLock& operator=(DataLock&& other) noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


class DataLockObserver {
public:
    DataLockObserver();
    DataLockObserver(
        const char* engineId,
        const char* dataDir,
        const std::function<void()>& lockAboutToBeCreated,
        const std::function<void()>& lockRemoved);

    ~DataLockObserver();

    DataLockObserver(const DataLockObserver&) = delete;
    DataLockObserver& operator=(const DataLockObserver&) = delete;

    DataLockObserver(DataLockObserver&& other) noexcept;
    DataLockObserver& operator=(DataLockObserver&& other) noexcept;

    bool getIsDataLocked() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


}
