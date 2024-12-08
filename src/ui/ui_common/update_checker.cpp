
#include "update_checker.h"

#include <cassert>
#include <chrono>
#include <future>
#include <optional>
#include <stdexcept>
#include <string>

#include "dpso_json/json.h"

#include "dpso_net/error.h"
#include "dpso_net/get_data.h"

#include "dpso_utils/error_set.h"
#include "dpso_utils/version_cmp.h"

#include "update_checker_platform.h"


using namespace dpso;
using namespace ui::updateChecker;


namespace {


class UpdateCheckerError : public std::runtime_error {
    using runtime_error::runtime_error;
};


class UpdateCheckerNetworkConnectionError
        : public UpdateCheckerError {
    using UpdateCheckerError::UpdateCheckerError;
};


struct UpdateInfo {
    std::string newVersion;
    std::string latestVersion;
    std::vector<UnmetRequirement> unmetRequirements;
};


UpdateInfo getUpdateInfoFromJson(
    const char* appVersion, const char* jsonData)
{
    const auto versionInfos = json::Array::load(jsonData);

    UpdateInfo result;

    const VersionCmp appVersionCmp{appVersion};
    VersionCmp newVersionCmp;
    VersionCmp latestVersionCmp;

    for (std::size_t i = 0; i < versionInfos.getSize(); ++i) {
        const auto versionInfo = versionInfos.getObject(i);

        try {
            const auto version = versionInfo.getStr("version");
            const VersionCmp versionCmp{version.c_str()};

            if (!(appVersionCmp < versionCmp)
                    || versionCmp < newVersionCmp)
                continue;

            try {
                result.unmetRequirements = processRequirements(
                    versionInfo.getObject("requirements"));
            } catch (RequirementsError& e) {
                throw UpdateCheckerError{str::format(
                    "Can't check requirements for version \"{}\" "
                    "(info index {}): {}",
                    version, i, e.what())};
            }

            if (result.unmetRequirements.empty()) {
                result.newVersion = version;
                newVersionCmp = versionCmp;
            }

            if (latestVersionCmp < versionCmp) {
                result.latestVersion = version;
                latestVersionCmp = versionCmp;
            }
        } catch (json::Error& e) {
            throw json::Error{str::format(
                "Version info at index {}: {}", i, e.what())};
        }
    }

    return result;
}


UpdateInfo getUpdateInfo(
    const char* appVersion,
    const char* userAgent,
    const char* infoFileUrl)
{
    std::string jsonData;
    try {
        jsonData = net::getData(infoFileUrl, userAgent);
    } catch (net::Error& e) {
        const auto message = str::format(
            "Network error: {}", e.what());

        try {
            throw;
        } catch (net::ConnectionError&) {
            throw UpdateCheckerNetworkConnectionError{message};
        } catch (net::Error&) {
            throw UpdateCheckerError{message};
        }
    }

    try {
        return getUpdateInfoFromJson(appVersion, jsonData.c_str());
    } catch (json::Error& e) {
        throw UpdateCheckerError{str::format(
            "JSON error: {}", e.what())};
    }
}


std::vector<UiUpdateCheckerUnmetRequirement> toPublic(
    const std::vector<UnmetRequirement>& unmetRequirements)
{
    std::vector<UiUpdateCheckerUnmetRequirement> result;

    result.reserve(unmetRequirements.size());
    for (const auto& ur : unmetRequirements)
        result.push_back({ur.required.c_str(), ur.actual.c_str()});

    return result;
}


}


struct UiUpdateChecker {
    std::string appVersion;
    std::string userAgent;
    std::string infoFileUrl;

    std::future<UpdateInfo> future{};

    std::optional<UiUpdateCheckerStatus> status{};
    std::string errorText{};

    UpdateInfo updateInfo{};
    std::vector<UiUpdateCheckerUnmetRequirement>
        publicUnmentRequirements{};
};



bool uiUpdateCheckerIsAvailable(void)
{
    return true;
}


UiUpdateChecker* uiUpdateCheckerCreate(
    const char* appVersion,
    const char* userAgent,
    const char* infoFileUrl)
{
    return new UiUpdateChecker{appVersion, userAgent, infoFileUrl};
}


void uiUpdateCheckerDelete(UiUpdateChecker* updateChecker)
{
    delete updateChecker;
}


void uiUpdateCheckerStartCheck(UiUpdateChecker* updateChecker)
{
    if (!updateChecker || updateChecker->future.valid())
        return;

    updateChecker->status.reset();

    updateChecker->future = std::async(
        std::launch::async,
        getUpdateInfo,
        updateChecker->appVersion.c_str(),
        updateChecker->userAgent.c_str(),
        updateChecker->infoFileUrl.c_str());
}


bool uiUpdateCheckerIsCheckInProgress(
    const UiUpdateChecker* updateChecker)
{
    return updateChecker
        && updateChecker->future.valid()
        && updateChecker->future.wait_for(std::chrono::seconds{})
            == std::future_status::timeout;
}


UiUpdateCheckerStatus uiUpdateCheckerGetUpdateInfo(
    UiUpdateChecker* updateChecker,
    UiUpdateCheckerUpdateInfo* updateInfo)
{
    if (!updateChecker) {
        setError("updateChecker is null");
        return UiUpdateCheckerStatusGenericError;
    }

    if (!updateInfo) {
        setError("updateInfo is null");
        return UiUpdateCheckerStatusGenericError;
    }

    if (!updateChecker->status) {
        if (!updateChecker->future.valid()) {
            setError("Update was not started");
            return UiUpdateCheckerStatusGenericError;
        }

        try {
            updateChecker->updateInfo = updateChecker->future.get();

            updateChecker->publicUnmentRequirements = toPublic(
                updateChecker->updateInfo.unmetRequirements);
            updateChecker->status = UiUpdateCheckerStatusSuccess;
        } catch (UpdateCheckerNetworkConnectionError& e) {
            updateChecker->status =
                UiUpdateCheckerStatusNetworkConnectionError;
            updateChecker->errorText = e.what();
        } catch (UpdateCheckerError& e) {
            updateChecker->status = UiUpdateCheckerStatusGenericError;
            updateChecker->errorText = e.what();
        }
    }

    assert(updateChecker->status);
    if (*updateChecker->status != UiUpdateCheckerStatusSuccess) {
        setError(
            "Can't process update info from \"{}\": {}",
            updateChecker->infoFileUrl,
            updateChecker->errorText);
        return *updateChecker->status;
    }

    *updateInfo = {
        updateChecker->updateInfo.newVersion.c_str(),
        updateChecker->updateInfo.latestVersion.c_str(),
        updateChecker->publicUnmentRequirements.data(),
        updateChecker->publicUnmentRequirements.size()};

    return UiUpdateCheckerStatusSuccess;
}
