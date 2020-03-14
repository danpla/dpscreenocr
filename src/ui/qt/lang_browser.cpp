
#include "lang_browser.h"

#include <QHeaderView>

#include "dpso/dpso.h"
#include "dpso_utils/dpso_utils.h"
#include "dpso_utils/intl.h"

#include "common/common.h"


LangBrowser::LangBrowser(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderLabels(
        QStringList()
        // Checkbox
        << ""
        << pgettext("language", "Name")
        << pgettext("language", "Code"));

    setSortingEnabled(true);
    sortByColumn(columnIdxName, Qt::AscendingOrder);

    setAllColumnsShowFocus(true);
    setRootIsDecorated(false);

    header()->setStretchLastSection(false);

    #if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    header()->setSectionResizeMode(
        columnIdxCheckbox, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(
        columnIdxName, QHeaderView::Stretch);
    header()->setSectionsMovable(false);
    #else
    header()->setResizeMode(
        columnIdxCheckbox, QHeaderView::ResizeToContents);
    header()->setResizeMode(columnIdxName, QHeaderView::Stretch);
    header()->setMovable(false);
    #endif

    connect(
        this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
        this, SLOT(toggleLang(QTreeWidgetItem*)));
}


void LangBrowser::toggleLang(QTreeWidgetItem* item)
{
    const auto langIdx = item->data(
        columnIdxCheckbox, Qt::UserRole).toInt();
    const auto state = (
        item->checkState(columnIdxCheckbox) == Qt::Checked);
    dpsoSetLangIsActive(langIdx, state);
}


void LangBrowser::loadState()
{
    dpsoCfgLoadActiveLangs(cfgKeyOcrLanguages, "eng");

    clear();
    setSortingEnabled(false);
    for (int i = 0, n = dpsoGetNumLangs(); i < n; ++i) {
        const auto* langCode = dpsoGetLangCode(i);
        const auto state = dpsoGetLangIsActive(i);

        auto* item = new QTreeWidgetItem(this);

        item->setData(columnIdxCheckbox, Qt::UserRole, i);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(
            columnIdxCheckbox, state ? Qt::Checked : Qt::Unchecked);

        item->setText(
            columnIdxName,
            dpsoGetLangName(langCode));
        item->setText(columnIdxCode, langCode);
    }
    setSortingEnabled(true);

    const auto columnIdx = qBound(
        static_cast<int>(columnIdxName),
        dpsoCfgGetInt(cfgKeyUiLanguagesSortColumn, columnIdxName),
        static_cast<int>(columnIdxCode));

    Qt::SortOrder sortOrder;
    if (dpsoCfgGetBool(cfgKeyUiLanguagesSortDescending, false))
        sortOrder = Qt::DescendingOrder;
    else
        sortOrder = Qt::AscendingOrder;

    sortByColumn(columnIdx, sortOrder);
}


void LangBrowser::saveState() const
{
    dpsoCfgSetInt(
        cfgKeyUiLanguagesSortColumn,
        header()->sortIndicatorSection());
    dpsoCfgSetBool(
        cfgKeyUiLanguagesSortDescending,
        header()->sortIndicatorOrder() == Qt::DescendingOrder);

    dpsoCfgSaveActiveLangs(cfgKeyOcrLanguages);
}
