
#include "ocr_data_lock.h"

#include <cassert>
#include <string>
#include <vector>


namespace dpso::ocr {
namespace {


struct ObserverData {
    std::function<void()> lockAboutToBeCreated;
    std::function<void()> lockRemoved;
};


struct SharedData {
    bool isDataLocked;
    std::vector<std::reference_wrapper<ObserverData>> observerDatas;

    static std::shared_ptr<SharedData> get(
        const char* engineId, const char* dataDir)
    {
        struct CacheEntry {
            std::string engineId;
            std::string dataDir;
            std::weak_ptr<SharedData> sharedData;
        };

        static std::vector<CacheEntry> cache;

        for (const auto& entry : cache) {
            if (entry.engineId != engineId
                    || entry.dataDir != dataDir)
                continue;

            if (auto sd = entry.sharedData.lock())
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
                for (auto iter = cache.begin();
                        iter < cache.end();
                        ++iter)
                    if (iter->engineId == engineId
                            && iter->dataDir == dataDir) {
                        cache.erase(iter);
                        break;
                    }

                delete sd;
            }};

        cache.push_back({engineId, dataDir, sd});

        return sd;
    }
};


}


struct DataLock::Impl {
    std::shared_ptr<SharedData> sd;

    Impl(const char* engineId, const char* dataDir)
        : sd{SharedData::get(engineId, dataDir)}
    {
        if (sd->isDataLocked)
            throw DataLock::DataLockedError{"Data is already locked"};

        for (auto& observerData : sd->observerDatas)
            if (auto& fn = observerData.get().lockAboutToBeCreated)
                fn();

        sd->isDataLocked = true;
    }

    ~Impl()
    {
        assert(sd->isDataLocked);
        sd->isDataLocked = false;

        for (auto& observerData : sd->observerDatas)
            if (auto& fn = observerData.get().lockRemoved)
                fn();
    }
};


DataLock::DataLock() = default;


DataLock::DataLock(const char* engineId, const char* dataDir)
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
            const char* engineId,
            const char* dataDir,
            const std::function<void()>& lockAboutToBeCreated,
            const std::function<void()>& lockRemoved)
        : sd{SharedData::get(engineId, dataDir)}
        , data{lockAboutToBeCreated, lockRemoved}
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
        const char* engineId,
        const char* dataDir,
        const std::function<void()>& lockAboutToBeCreated,
        const std::function<void()>& lockRemoved)
    : impl{std::make_unique<Impl>(
        engineId, dataDir, lockAboutToBeCreated, lockRemoved)}
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
