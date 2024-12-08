
#include "update_check.h"

#include <QDesktopServices>
#include <QDialog>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVBoxLayout>

#include "dpso_ext/dpso_ext.h"
#include "dpso_intl/dpso_intl.h"
#include "dpso_utils/dpso_utils.h"

#include "utils.h"


#define _(S) gettext(S)


namespace ui::qt {
namespace {


class UpdateCheckProgressDialog : public QDialog {
    Q_OBJECT
public:
    UpdateCheckProgressDialog(
        QWidget* parent, const UiUpdateChecker* updateChecker);

    void reject() override
    {
        // Do nothing to prevent the dialog from closing.
    }
protected:
    void timerEvent(QTimerEvent* event) override;
private:
    const UiUpdateChecker* updateChecker;
};


UpdateCheckProgressDialog::UpdateCheckProgressDialog(
        QWidget* parent, const UiUpdateChecker* updateChecker)
    : QDialog{parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint}
    , updateChecker{updateChecker}
{
    setWindowTitle(uiAppName);

    auto* label = new QLabel(_("Checking for updates\342\200\246"));

    auto* progressBar = new QProgressBar();
    progressBar->setRange(0, 0);  // Use the "indeterminate" mode.
    progressBar->setTextVisible(false);

    auto* layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->addWidget(label);
    layout->addWidget(progressBar);

    startTimer(1000 / 30);
}


void UpdateCheckProgressDialog::timerEvent(QTimerEvent* event)
{
    (void)event;

    if (!uiUpdateCheckerIsCheckInProgress(updateChecker))
        accept();
}


}


void runUpdateCheckProgressDialog(
    QWidget* parent, const UiUpdateChecker* updateChecker)
{
    UpdateCheckProgressDialog dialog(parent, updateChecker);
    dialog.exec();
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


static QString getUnmetRequirementsText(
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


#include "update_check.moc"
