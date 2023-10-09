
#include "lang_manager/install_progress_dialog.h"

#include <string>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QFrame>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#include "dpso_ext/dpso_ext.h"
#include "dpso_intl/dpso_intl.h"
#include "ui_common/ui_common.h"

#include "utils.h"


#define _(S) gettext(S)


namespace ui::qt::langManager {
namespace {


// We have to write our own label that elides text since QLabel has no
// such feature.
class ProgressLabel : public QFrame {
    Q_OBJECT
public:
    QSize minimumSizeHint() const override
    {
        return QSize(0, fontMetrics().height());
    }

    QSize sizeHint() const override
    {
        return minimumSizeHint();
    }

    void setText(const QString& newText)
    {
        text = newText;
        update();
    }
protected:
    void paintEvent(QPaintEvent* event) override
    {
        QFrame::paintEvent(event);

        QPainter painter(this);
        painter.drawText(
            rect(),
            Qt::AlignCenter,
            fontMetrics().elidedText(text, Qt::ElideRight, width()));
    }
private:
    QString text;
};


// Qt already has QProgressDialog, but its default behavior makes it
// hard (if not impossible) to show confirmation before canceling the
// operation. It also uses QLabel under the hood, which causes the
// dialog to dynamically change its width when the new text is set.
class InstallProgressDialog : public QDialog {
    Q_OBJECT
public:
    InstallProgressDialog(
        DpsoOcrLangManager* langManager,
        InstallMode installMode,
        QWidget* parent);

    void reject() override;
protected:
    void timerEvent(QTimerEvent* event) override;
private:
    DpsoOcrLangManager* langManager;
    InstallMode installMode;
    ProgressLabel* label;
    QProgressBar* progressBar;

    bool confirmCancellation();
};


InstallProgressDialog::InstallProgressDialog(
            DpsoOcrLangManager* langManager,
            InstallMode installMode,
            QWidget* parent)
    : QDialog{
        parent,
        Qt::WindowTitleHint
            | Qt::WindowSystemMenuHint
            | Qt::WindowCloseButtonHint}
    , langManager{langManager}
    , installMode{installMode}
{
    setWindowTitle(uiAppName);

    label = new ProgressLabel();
    progressBar = new QProgressBar();

    auto* buttonBox = new QDialogButtonBox();
    QObject::connect(
        buttonBox, &QDialogButtonBox::rejected,
        this, &InstallProgressDialog::reject);

    auto* cancelButton = buttonBox->addButton(
        _("Cancel\342\200\246"), QDialogButtonBox::RejectRole);
    cancelButton->setDefault(true);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(label);
    layout->addWidget(progressBar);
    layout->addWidget(buttonBox);

    startTimer(1000 / 30);
}


bool InstallProgressDialog::confirmCancellation()
{
    if (installMode == InstallMode::install)
        return confirmDestructiveAction(
            this,
            _("Are you sure you want to cancel the installation?"),
            _("Continue installation"),
            _("Cancel installation"));
    else
        return confirmDestructiveAction(
            this,
            _("Are you sure you want to cancel the update?"),
            _("Continue updating"),
            _("Cancel update"));
}



void InstallProgressDialog::reject()
{
    if (!confirmCancellation())
        return;

    dpsoOcrLangManagerCancelInstall(langManager);
    QDialog::reject();
}


void InstallProgressDialog::timerEvent(QTimerEvent* event)
{
    (void)event;

    DpsoOcrLangInstallProgress progress;
    dpsoOcrLangManagerGetInstallProgress(langManager, &progress);

    if (progress.totalLangs == 0) {
        accept();
        return;
    }

    const auto newValue =
        ((progress.curLangNum - 1) * 100 + progress.curLangProgress)
        / progress.totalLangs;

    if (newValue == progressBar->value())
        return;

    progressBar->setValue(newValue);

    const auto* langCode = dpsoOcrLangManagerGetLangCode(
        langManager, progress.curLangIdx);
    const auto* langName = dpsoOcrLangManagerGetLangName(
        langManager, progress.curLangIdx);

    label->setText(dpsoStrNFormat(
        installMode == InstallMode::install
            ? _("Installing \342\200\234{name}\342\200\235 "
                "({current}/{total})")
            : _("Updating \342\200\234{name}\342\200\235 "
                "({current}/{total})"),
        {
            {"name", *langName ? gettext(langName) : langCode},
            {"current", std::to_string(progress.curLangNum).c_str()},
            {"total", std::to_string(progress.totalLangs).c_str()},
        }));
}


}


void runInstallProgressDialog(
    DpsoOcrLangManager* langManager,
    InstallMode installMode,
    QWidget* parent)
{
    InstallProgressDialog dialog(langManager, installMode, parent);
    dialog.exec();
}


}


#include "install_progress_dialog.moc"
