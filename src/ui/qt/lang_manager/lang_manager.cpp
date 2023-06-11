
#include "lang_manager/lang_manager.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include "dpso_intl/dpso_intl.h"
#include "dpso_utils/dpso_utils.h"
#include "ui_common/ui_common.h"

#include "lang_manager/lang_fetch_progress_dialog.h"
#include "lang_manager/lang_list.h"
#include "lang_manager/lang_manager_page.h"
#include "lang_manager/lang_op_status_error.h"
#include "utils.h"


#define _(S) gettext(S)


namespace ui::qt::langManager {


static void fetchExternalLangs(
    DpsoOcrLangManager* langManager, QWidget* parent)
{
    if (!dpsoOcrLangManagerStartFetchExternalLangs(langManager)) {
        QMessageBox::critical(
            parent,
            uiAppName,
            QString("Can't start fetching the language list: ")
                + dpsoGetError());
        return;
    }

    runFetchLangsProgressDialog(langManager, parent);

    DpsoOcrLangOpStatus status;
    dpsoOcrLangManagerGetFetchExternalLangsStatus(
        langManager, &status);

    if (dpsoOcrLangOpStatusIsError(status.code)) {
        showLangOpStatusError(
            parent,
            _("An error occurred while fetching the list of "
                "languages"),
            status);
        return;
    }

    if (!dpsoOcrLangManagerLoadFetchedExternalLangs(langManager))
        QMessageBox::critical(
            parent,
            uiAppName,
            QString("Can't load the language list: ")
                + dpsoGetError());
}


void runLangManager(
    int ocrEngineIdx, const std::string& dataDir, QWidget* parent)
{
    dpso::OcrLangManagerUPtr langManager{
        dpsoOcrLangManagerCreate(ocrEngineIdx, dataDir.c_str())};
    if (!langManager) {
        QMessageBox::critical(
            parent,
            uiAppName,
            QString("Can't create language manager: ")
                + dpsoGetError());
        return;
    }

    dpsoOcrLangManagerSetUserAgent(
        langManager.get(), uiGetUserAgent());

    fetchExternalLangs(langManager.get(), parent);

    QDialog dialog(
        parent,
        Qt::WindowTitleHint
            | Qt::WindowSystemMenuHint
            | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle(_("Language manager"));

    auto* langList = new LangList(langManager.get(), &dialog);

    auto* tabs = new QTabWidget();
    tabs->addTab(
        createLangManagerInstallPage(*langList), _("Install"));
    tabs->addTab(createLangManagerUpdatePage(*langList), _("Update"));
    tabs->addTab(createLangManagerRemovePage(*langList), _("Remove"));

    auto* buttonBox = new QDialogButtonBox();
    QObject::connect(
        buttonBox, &QDialogButtonBox::rejected,
        &dialog, &QDialog::reject);

    auto* closeButton = buttonBox->addButton(
        _("Close"), QDialogButtonBox::RejectRole);
    closeButton->setDefault(true);

    auto* layout = new QVBoxLayout(&dialog);
    layout->addWidget(tabs);
    layout->addWidget(buttonBox);

    dialog.exec();
}


}
