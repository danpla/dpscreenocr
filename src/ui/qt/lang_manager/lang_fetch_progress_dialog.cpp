
#include "lang_manager/lang_fetch_progress_dialog.h"

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QVboxLayout>

#include "dpso_intl/dpso_intl.h"
#include "ui_common/ui_common.h"


#define _(S) gettext(S)


namespace langManager {
namespace {


class LangFetchProgressDialog : public QDialog {
    Q_OBJECT
public:
    LangFetchProgressDialog(
        const DpsoOcrLangManager* langManager, QWidget* parent);

    void reject() override
    {
        // Do nothing to prevent the dialog from closing.
    }
protected:
    void timerEvent(QTimerEvent* event) override;
private:
    const DpsoOcrLangManager* langManager;
};


LangFetchProgressDialog::LangFetchProgressDialog(
        const DpsoOcrLangManager* langManager,
        QWidget* parent)
    : QDialog{
        parent,
        Qt::CustomizeWindowHint | Qt::WindowTitleHint}
    , langManager{langManager}
{
    setWindowTitle(uiAppName);

    auto* label = new QLabel(
        _("Fetching the language list\342\200\246"));

    auto* progressBar = new QProgressBar();
    progressBar->setRange(0, 0);  // Use the "indeterminate" mode.
    progressBar->setTextVisible(false);

    auto* layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->addWidget(label);
    layout->addWidget(progressBar);

    startTimer(1000 / 30);
}


void LangFetchProgressDialog::timerEvent(QTimerEvent* event)
{
    (void)event;

    DpsoOcrLangOpStatus status;
    dpsoOcrLangManagerGetFetchExternalLangsStatus(
        langManager, &status);

    if (status.code != DpsoOcrLangOpStatusCodeProgress)
        accept();
}


}


void runFetchLangsProgressDialog(
    const DpsoOcrLangManager* langManager,
    QWidget* parent)
{
    LangFetchProgressDialog dialog(langManager, parent);
    dialog.exec();
}


}


#include "lang_fetch_progress_dialog.moc"
