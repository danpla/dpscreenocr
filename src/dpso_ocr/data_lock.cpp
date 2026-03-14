#include "data_lock.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>


namespace dpso::ocr {
namespace {


struct ObserverData {
    std::function<void()> beforeLockCreated;
    std::function<void()> afterLockRemoved;
};


struct SharedData {
    bool isDataLocked;
    std::vector<std::reference_wrapper<ObserverData>> observerDatas;

    static std::shared_ptr<SharedData> get(
        std::string_view engineId, std::string_view dataDir)
    {
        struct CacheEntry {
            std::string engineId;
            std::string dataDir;
            std::weak_ptr<SharedData> sharedData;
        };

        static std::vector<CacheEntry> cache;

        static const auto findCacheEntry = [](
            std::string_view engineId, std::string_view dataDir)
        {
            return std::find_if(
                cache.begin(), cache.end(),
                [&](const CacheEntry& entry)
                {
                    return entry.engineId == engineId
                        && entry.dataDir == dataDir;
                });
        };

        if (const auto iter = findCacheEntry(engineId, dataDir);
                iter != cache.end()) {
            if (auto sd = iter->sharedData.lock())
                return sd;

            assert(false);
        }

        std::shared_ptr<SharedData> sd{
            new SharedData{},
            [
                engineId = std::string{engineId},
                dataDir = std::string{dataDir}]
            (SharedData* sd)
            {
                const auto iter = findCacheEntry(engineId, dataDir);
                if (iter != cache.end())
                    cache.erase(iter);

                delete sd;
            }};

        cache.push_back(
            {std::string{engineId}, std::string{dataDir}, sd});

        return sd;
    }
};


}


struct DataLock::Impl {
    std::shared_ptr<SharedData> sd;

    Impl(std::string_view engineId, std::string_view dataDir)
        : sd{SharedData::get(engineId, dataDir)}
    {
        if (sd->isDataLocked)
            throw DataLock::DataLockedError{"Data is already locked"};

        for (auto& observerData : sd->observerDatas)
            if (auto& fn = observerData.get().beforeLockCreated)
                fn();

        sd->isDataLocked = true;
    }

    ~Impl()
    {
        assert(sd->isDataLocked);
        sd->isDataLocked = false;

        for (auto& observerData : sd->observerDatas)
            if (auto& fn = observerData.get().afterLockRemoved)
                fn();
    }
};


DataLock::DataLock() = default;


DataLock::DataLock(
        std::string_view engineId, std::string_view dataDir)
    : impl{std::make_unique<Impl>(engineId, dataDir)}
{
}


DataLock::~DataLock() = default;
DataLock::DataLock(DataLock&&) noexcept = default;
DataLock& DataLock::operator=(DataLock&&) noexcept = default;


struct DataLockObserver::Impl {
    std::shared_ptr<SharedData> sd;
    ObserverData data;

    Impl(
            std::string_view engineId,
            std::string_view dataDir,
            const std::function<void()>& beforeLockCreated,
            const std::function<void()>& afterLockRemoved)
        : sd{SharedData::get(engineId, dataDir)}
        , data{beforeLockCreated, afterLockRemoved}
    {
        sd->observerDatas.push_back(data);
    }

    ~Impl()
    {
        auto& datas = sd->observerDatas;

        for (auto iter = datas.begin(); iter < datas.end(); ++iter)
            if (&iter->get() == &data) {
                datas.erase(iter);
                break;
            }
    }
};


DataLockObserver::DataLockObserver() = default;


DataLockObserver::DataLockObserver(
        std::string_view engineId,
        std::string_view dataDir,
        const std::function<void()>& beforeLockCreated,
        const std::function<void()>& afterLockRemoved)
    : impl{std::make_unique<Impl>(
        engineId, dataDir, beforeLockCreated, afterLockRemoved)}
{
}


DataLockObserver::~DataLockObserver() = default;
DataLockObserver::DataLockObserver(
    DataLockObserver&&) noexcept = default;
DataLockObserver& DataLockObserver::operator=(
    DataLockObserver&&) noexcept = default;


bool DataLockObserver::getIsDataLocked() const
{
    return impl && impl->sd->isDataLocked;
}


}
