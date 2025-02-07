#include "update_checker.h"

#include <QApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "dpso/dpso.h"
#include "dpso_intl/dpso_intl.h"
#include "dpso_utils/dpso_utils.h"

#include "utils.h"


#define _(S) gettext(S)


namespace ui::qt {
namespace {


std::optional<std::tm> getCurLocalTime()
{
    const auto time = std::time(nullptr);
    if (const auto* tm = std::localtime(&time))
        return *tm;

    return {};
}


void showUpdateCheckError(
    QWidget* parent, UiUpdateCheckerStatus status)
{
    QString informativeText;

    switch (status) {
    case UiUpdateCheckerStatusSuccess:
        return;
    case UiUpdateCheckerStatusGenericError:
        break;
    case UiUpdateCheckerStatusNetworkConnectionError:
        informativeText =
            _("Check your network connection and try again.");
        break;
    }

    showError(
        parent,
        _("An error occurred while checking for updates"),
        informativeText,
        dpsoGetError());
}


QString getUnmetRequirementsText(
    const UiUpdateCheckerUpdateInfo& updateInfo)
{
    QStringList list;

    for (size_t i = 0; i < updateInfo.numUnmetRequirements; ++i) {
        const auto& ur = updateInfo.unmetRequirements[i];
        if (*ur.actual)
            list.append(dpsoStrNFormat(
                _("{required} (you have {actual})"),
                {
                    {"required", ur.required},
                    {"actual", ur.actual}}));
        else
            list.append(ur.required);
    }

    list.sort();

    return joinInLayoutDirection(", ", list);
}


void showUpdateInfo(
    QWidget* parent, const UiUpdateCheckerUpdateInfo& updateInfo)
{
    // Center the update info dialog on the screen if it's shown when
    // the main window is hidden.
    if (parent && !parent->isVisible())
        parent = nullptr;

    QString text;
    QString informativeText;

    if (*updateInfo.newVersion) {
        text = dpsoStrNFormat(
            _("{app_name} {new_version} is available (you have "
                "{current_version})"),
            {
                {"app_name", uiAppName},
                {"new_version", updateInfo.newVersion},
                {"current_version", uiAppVersion}});

        informativeText =
            _("Visit the website for more information.");
    } else
        text = dpsoStrNFormat(
            updateInfo.numUnmetRequirements == 0
                ? _("You have the latest version of {app_name}")
                : _("You have the latest version of {app_name} "
                    "available for your system"),
            {{"app_name", uiAppName}});

    if (updateInfo.numUnmetRequirements > 0) {
        if (!informativeText.isEmpty())
            informativeText += "\n\n";

        informativeText = dpsoStrNFormat(
            _("The latest version is {latest_version}, but your "
                "system does not meet the following minimum "
                "requirements: {requirements}."),
            {
                {"latest_version", updateInfo.latestVersion},
                {
                    "requirements",
                    getUnmetRequirementsText(updateInfo)
                        .toUtf8().data()}});
    }

    QMessageBox msgBox(
        QMessageBox::NoIcon,
        uiAppName,
        text,
        QMessageBox::NoButton,
        parent);

    msgBox.setInformativeText(informativeText);

    QPushButton* webisteButton{};
    if (*updateInfo.newVersion)
        webisteButton = msgBox.addButton(
            _("Website"), QMessageBox::AcceptRole);

    msgBox.addButton(_("Close"), QMessageBox::RejectRole);

    msgBox.exec();

    if (webisteButton && msgBox.clickedButton() == webisteButton)
        QDesktopServices::openUrl(QUrl(uiAppWebsite));
}


}


UpdateChecker::UpdateChecker(QWidget* parent)
    : QObject{parent}
    , parentWidget{parent}
{
}


bool UpdateChecker::getAutoCheckIsEnabled() const
{
    return autoCheck;
}


void UpdateChecker::setAutoCheckIsEnabled(bool isEnabled)
{
    autoCheck = isEnabled;

    if (!autoCheck)
        stopAutoCheck();

    // The automatic check should only happen at the application
    // startup, so do nothing if autoCheck is true.
}


int UpdateChecker::getAutoCheckIntervalDays() const
{
    return autoCheckIntervalDays;
}


void UpdateChecker::loadState(const DpsoCfg* cfg)
{
    autoCheck = dpsoCfgGetBool(
        cfg, cfgKeyUpdateCheckAuto, cfgDefaultValueUpdateCheckAuto);
    autoCheckIntervalDays = qBound(
        1,
        dpsoCfgGetInt(
            cfg,
            cfgKeyUpdateCheckAutoIntervalDays,
            cfgDefaultValueUpdateCheckAutoIntervalDays),
        365);

    lastCheckTime.reset();
    std::tm time;
    if (dpsoCfgGetTime(cfg, cfgKeyUpdateCheckLastTime, &time))
        lastCheckTime = time;

    handleAutoCheck();
}


void UpdateChecker::saveState(DpsoCfg* cfg) const
{
    dpsoCfgSetBool(cfg, cfgKeyUpdateCheckAuto, autoCheck);
    dpsoCfgSetInt(
        cfg,
        cfgKeyUpdateCheckAutoIntervalDays,
        autoCheckIntervalDays);
    if (lastCheckTime)
        dpsoCfgSetTime(
            cfg, cfgKeyUpdateCheckLastTime, &*lastCheckTime);
    else
        dpsoCfgSetStr(cfg, cfgKeyUpdateCheckLastTime, "");
}


void UpdateChecker::timerEvent(QTimerEvent* event)
{
    (void)event;

    Q_ASSERT(autoCheck);
    Q_ASSERT(autoChecker);

    if (uiUpdateCheckerIsCheckInProgress(autoChecker.get()))
        return;

    UiUpdateCheckerUpdateInfo updateInfo;
    const auto status = uiUpdateCheckerGetUpdateInfo(
        autoChecker.get(), &updateInfo);

    if (status == UiUpdateCheckerStatusSuccess) {
        if (*updateInfo.newVersion) {
            // Postpone the report if the user is busy.
            if (QApplication::activeModalWidget()
                    || QApplication::activePopupWidget()
                    || dpsoSelectionGetIsEnabled())
                return;

            showUpdateInfo(parentWidget, updateInfo);
        }

        lastCheckTime = getCurLocalTime();
    }

    stopAutoCheck();
}


void UpdateChecker::checkUpdates()
{
    stopAutoCheck();

    UpdateCheckerUPtr checker{uiUpdateCheckerCreateDefault()};
    if (!checker) {
        QMessageBox::critical(
            parentWidget,
            uiAppName,
            QString("Can't create update checker: ")
                + dpsoGetError());
        return;
    }

    uiUpdateCheckerStartCheck(checker.get());

    showProgressDialog(
        parentWidget,
        _("Checking for updates\342\200\246"),
        [&]()
        {
            return uiUpdateCheckerIsCheckInProgress(checker.get());
        });

    UiUpdateCheckerUpdateInfo updateInfo;
    const auto status = uiUpdateCheckerGetUpdateInfo(
        checker.get(), &updateInfo);

    if (status == UiUpdateCheckerStatusSuccess) {
        showUpdateInfo(parentWidget, updateInfo);
        lastCheckTime = getCurLocalTime();
    } else
        showUpdateCheckError(parentWidget, status);
}


void UpdateChecker::handleAutoCheck()
{
    stopAutoCheck();

    if (!autoCheck || !uiUpdateCheckerIsAvailable())
        return;

    if (lastCheckTime) {
        const auto lastTime = std::mktime(&*lastCheckTime);
        const auto curTime = std::time(nullptr);

        const auto autoCheckIntervalSec =
            static_cast<double>(autoCheckIntervalDays)
            * 24 * 60 * 60;

        if (lastTime != -1
                && curTime != -1
                && std::difftime(curTime, lastTime)
                    < autoCheckIntervalSec)
            return;
    }

    autoChecker.reset(uiUpdateCheckerCreateDefault());
    if (!autoChecker) {
        qWarning(
            "uiUpdateCheckerCreateDefault(): %s", dpsoGetError());
        return;
    }

    uiUpdateCheckerStartCheck(autoChecker.get());

    timerId = startTimer(3000, Qt::VeryCoarseTimer);
}


void UpdateChecker::stopAutoCheck()
{
    if (timerId != 0) {
        killTimer(timerId);
        timerId = 0;
    }

    autoChecker.reset();
}


}
