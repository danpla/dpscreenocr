#include "lang_manager/lang_manager.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include "dpso_intl/dpso_intl.h"
#include "dpso_ocr/dpso_ocr.h"
#include "dpso_utils/dpso_utils.h"
#include "ui_common/ui_common.h"

#include "lang_manager/lang_list.h"
#include "lang_manager/lang_manager_page.h"
#include "lang_manager/lang_op_status_error.h"
#include "utils.h"


#define _(S) gettext(S)


namespace ui::qt::langManager {


static void fetchExternalLangs(
    QWidget* parent, DpsoOcrLangManager* langManager)
{
    if (!dpsoOcrLangManagerStartFetchExternalLangs(langManager)) {
        QMessageBox::critical(
            parent,
            uiAppName,
            QString("Can't start fetching the language list: ")
                + dpsoGetError());
        return;
    }

    showProgressDialog(
        parent,
        _("Fetching the language list\342\200\246"),
        [&]
        {
            DpsoOcrLangOpStatus status;
            dpsoOcrLangManagerGetFetchExternalLangsStatus(
                langManager, &status);

            return status.code == DpsoOcrLangOpStatusCodeProgress;
        });

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


static QString getUpdateTabTitle(
    const DpsoOcrLangManager* langManager)
{
    int numUpdatable{};

    const auto numLangs = dpsoOcrLangManagerGetNumLangs(langManager);
    for (int i = 0; i < numLangs; ++i)
        if (dpsoOcrLangManagerGetLangState(langManager, i)
                == DpsoOcrLangStateUpdateAvailable)
            ++numUpdatable;

    return strNFormat(
        _("Update ({count})"), {{"count", numUpdatable}});
}


void runLangManager(
    QWidget* parent, int ocrEngineIdx, const std::string& dataDir)
{
    DpsoOcrEngineInfo ocrEngineInfo;
    dpsoOcrGetEngineInfo(ocrEngineIdx, &ocrEngineInfo);

    dpso::OcrLangManagerUPtr langManager{
        dpsoOcrLangManagerCreate(
            ocrEngineIdx,
            dataDir.c_str(),
            uiGetUserAgent(),
            uiGetOcrDataInfoFileUrl(&ocrEngineInfo))};
    if (!langManager) {
        QMessageBox::critical(
            parent,
            uiAppName,
            QString("Can't create language manager: ")
                + dpsoGetError());
        return;
    }

    fetchExternalLangs(parent, langManager.get());

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
    const auto updateTabIdx = tabs->addTab(
        createLangManagerUpdatePage(*langList), {});
    tabs->addTab(createLangManagerRemovePage(*langList), _("Remove"));

    const auto refreshUpdateTabTitle = [&]
    {
        tabs->setTabText(
            updateTabIdx, getUpdateTabTitle(langManager.get()));
    };

    refreshUpdateTabTitle();

    QObject::connect(
        langList,
        &QAbstractItemModel::modelReset,
        refreshUpdateTabTitle);

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
