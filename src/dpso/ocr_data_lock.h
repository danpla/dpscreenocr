
#pragma once

#include <functional>
#include <memory>


namespace dpso::ocr {


class DataLock {
public:
    static std::shared_ptr<DataLock> get(
        const char* engineId, const char* dataDir);
private:
    DataLock() = default;
};


class DataLockObserver {
public:
    DataLockObserver();
    DataLockObserver(
        const char* engineId,
        const char* dataDir,
        std::function<void()> lockAboutToBeCreated,
        std::function<void()> lockRemoved);

    ~DataLockObserver();

    DataLockObserver(DataLockObserver&& other) noexcept;
    DataLockObserver& operator=(DataLockObserver&& other) noexcept;

    bool getIsDataLocked() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


}
