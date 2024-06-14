
#include "lang_manager/lang_manager_page.h"

#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

#include "dpso/dpso.h"
#include "dpso_ext/dpso_ext.h"
#include "dpso_intl/dpso_intl.h"
#include "dpso_utils/dpso_utils.h"
#include "dpso_utils/str.h"
#include "ui_common/ui_common.h"

#include "lang_manager/install_mode.h"
#include "lang_manager/install_progress_dialog.h"
#include "lang_manager/lang_list.h"
#include "lang_manager/lang_list_sort_filter_proxy.h"
#include "lang_manager/lang_op_status_error.h"
#include "utils.h"


#define _(S) gettext(S)


namespace ui::qt::langManager {
namespace {


class LangManagerPage : public QWidget {
    Q_OBJECT
public:
    LangManagerPage(
        LangList& langList,
        LangListSortFilterProxy::LangGroup langGroup,
        const QString& actionName);
protected:
    // Returns false if the action was canceled.
    virtual bool performAction(
        DpsoOcrLangManager* langManager,
        const QList<QByteArray>& langCodes) = 0;
private slots:
    void selectionChanged();
    void actionActivated();
private:
    LangList* langList;
    QTreeView* treeView;
    QLabel* selectionInfoLabel;
    QPushButton* actionButton;
};


LangManagerPage::LangManagerPage(
        LangList& langList,
        LangListSortFilterProxy::LangGroup langGroup,
        const QString& actionName)
    : langList{&langList}
{
    auto* filterLineEdit = new QLineEdit();
    filterLineEdit->setPlaceholderText(pgettext("noun", "Filter"));
    filterLineEdit->setClearButtonEnabled(true);

    treeView = new QTreeView();
    treeView->setUniformRowHeights(true);
    treeView->setAllColumnsShowFocus(true);
    treeView->setRootIsDecorated(false);
    treeView->setSortingEnabled(true);
    treeView->sortByColumn(0, Qt::AscendingOrder);
    treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    auto* sortFilterProxyModel = new LangListSortFilterProxy(
        langGroup, this);
    sortFilterProxyModel->setSourceModel(&langList);

    connect(
        filterLineEdit,
        &QLineEdit::textChanged,
        sortFilterProxyModel,
        &LangListSortFilterProxy::setFilterText);

    treeView->setModel(sortFilterProxyModel);

    treeView->header()->setSectionsMovable(false);
    treeView->header()->setStretchLastSection(false);
    treeView->header()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    treeView->header()->setSectionResizeMode(
        LangListSortFilterProxy::columnIdxName, QHeaderView::Stretch);

    treeView->setSizeAdjustPolicy(
        QAbstractScrollArea::AdjustToContents);

    selectionInfoLabel = new QLabel();

    actionButton = new QPushButton(actionName);
    actionButton->setAutoDefault(false);
    actionButton->setEnabled(false);

    connect(
        actionButton, &QPushButton::clicked,
        this, &LangManagerPage::actionActivated);

    connect(
        treeView->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        &LangManagerPage::selectionChanged);

    selectionChanged();  // Initialize selectionInfoLabel text.

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(filterLineEdit);
    layout->addWidget(treeView, 1);
    layout->addWidget(selectionInfoLabel);
    layout->addWidget(actionButton);
}


void LangManagerPage::selectionChanged()
{
    const auto* selectionModel = treeView->selectionModel();

    actionButton->setEnabled(selectionModel->hasSelection());

    if (!selectionModel->hasSelection()) {
        selectionInfoLabel->setText(_("No languages selected."));
        return;
    }

    const auto selectedRows = selectionModel->selectedRows(
        LangListSortFilterProxy::columnIdxSize);

    int64_t totalSize{};
    for (const auto& row : selectedRows)
        totalSize += row.data(Qt::UserRole).value<int64_t>();

    selectionInfoLabel->setText(
        dpsoStrNFormat(
            ngettext(
                "{count} language selected ({size}).",
                "{count} languages selected ({size}).",
                selectedRows.size()),
            {
                {
                    "count",
                    dpso::str::toStr(selectedRows.size()).c_str()},
                {"size", formatDataSize(totalSize).toUtf8().data()}
            }));
}


void LangManagerPage::actionActivated()
{
    const auto selectedRows =
        treeView->selectionModel()->selectedRows(
            LangListSortFilterProxy::columnIdxCode);

    QList<QByteArray> langCodes;
    langCodes.reserve(selectedRows.size());
    for (const auto& row : selectedRows)
        langCodes.append(row.data().toString().toUtf8());

    if (!performAction(langList->getLangManager(), langCodes))
        return;

    langList->reloadLangs();

    // The QItemSelectionModel::selectionChanged signal is not emitted
    // when the model is reset, so refresh things manually.
    selectionChanged();
}


class LangManagerPageInstall : public LangManagerPage {
    Q_OBJECT
public:
    LangManagerPageInstall(
            LangList& langList, InstallMode installMode)
        : LangManagerPage{
            langList,
            installMode == InstallMode::install
                ? LangListSortFilterProxy::LangGroup::installable
                : LangListSortFilterProxy::LangGroup::updatable,
            installMode == InstallMode::install
                ? _("Install") : _("Update")}
        , installMode{installMode}
    {
    }
protected:
    bool performAction(
        DpsoOcrLangManager* langManager,
        const QList<QByteArray>& langCodes) override;
private:
    InstallMode installMode;
};


bool LangManagerPageInstall::performAction(
    DpsoOcrLangManager* langManager,
    const QList<QByteArray>& langCodes)
{
    for (const auto& langCode : langCodes)
        dpsoOcrLangManagerSetInstallMark(
            langManager,
            dpsoOcrLangManagerGetLangIdx(
                langManager, langCode.data()),
            true);

    if (!dpsoOcrLangManagerStartInstall(langManager)) {
        QMessageBox::warning(
            this,
            uiAppName,
            QString("Can't start installation: ")
                + dpsoGetError());
        return false;
    }

    runInstallProgressDialog(langManager, installMode, this);

    DpsoOcrLangOpStatus installStatus;
    dpsoOcrLangManagerGetInstallStatus(langManager, &installStatus);

    if (dpsoOcrLangOpStatusIsError(installStatus.code))
        showLangOpStatusError(
            this,
            installMode == InstallMode::install
                ? _("An error occurred while installing languages")
                : _("An error occurred while updating languages"),
            installStatus);

    return true;
}


class LangManagerPageRemove : public LangManagerPage {
    Q_OBJECT
public:
    explicit LangManagerPageRemove(LangList& langList)
        : LangManagerPage{
            langList,
            LangListSortFilterProxy::LangGroup::removable,
            _("Remove\342\200\246")}
    {
    }
protected:
    bool performAction(
        DpsoOcrLangManager* langManager,
        const QList<QByteArray>& langCodes) override;
};


bool LangManagerPageRemove::performAction(
    DpsoOcrLangManager* langManager,
    const QList<QByteArray>& langCodes)
{
    QString questionText;

    if (langCodes.size() == 1) {
        const auto* langCode = langCodes[0].data();

        const auto langIdx = dpsoOcrLangManagerGetLangIdx(
            langManager, langCode);
        const auto* langName = dpsoOcrLangManagerGetLangName(
            langManager, langIdx);

        questionText = dpsoStrNFormat(
            _("Remove \342\200\234{name}\342\200\235?"),
            {{"name", *langName ? gettext(langName) : langCode}});
    } else
        questionText = dpsoStrNFormat(
            ngettext(
                "Remove {count} selected language?",
                "Remove {count} selected languages?",
                langCodes.size()),
            {{"count", dpso::str::toStr(langCodes.size()).c_str()}});

    if (!confirmDestructiveAction(
            this, questionText, _("Cancel"), _("Remove")))
        return false;

    QString detailedErrorText;

    for (const auto& langCode : langCodes)
        if (!dpsoOcrLangManagerRemoveLang(
                langManager,
                dpsoOcrLangManagerGetLangIdx(
                    langManager, langCode.data()))) {
            if (!detailedErrorText.isEmpty())
                detailedErrorText += '\n';

            detailedErrorText += langCode + ": " + dpsoGetError();
        }

    if (!detailedErrorText.isEmpty())
        showError(
            this,
            _("Some languages were not removed"),
            {},
            detailedErrorText);

    return true;
}


}


QWidget* createLangManagerInstallPage(LangList& langList)
{
    return new LangManagerPageInstall(langList, InstallMode::install);
}


QWidget* createLangManagerUpdatePage(LangList& langList)
{
    return new LangManagerPageInstall(langList, InstallMode::update);
}


QWidget* createLangManagerRemovePage(LangList& langList)
{
    return new LangManagerPageRemove(langList);
}


}


#include "lang_manager_page.moc"
