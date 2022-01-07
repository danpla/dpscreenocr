
#include "lang_browser.h"

#include <QHeaderView>
#include <QTreeWidgetItem>

#include "dpso/dpso.h"
#include "dpso_intl/dpso_intl.h"
#include "dpso_utils/dpso_utils.h"

#include "common/common.h"


namespace {


enum ColumnIdx {
    columnIdxCheckbox,
    columnIdxName,
    columnIdxCode
};


}


LangBrowser::LangBrowser(QWidget* parent)
    : QTreeWidget{parent}
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
        this, SLOT(toggleLang(QTreeWidgetItem*, int)));
}


void LangBrowser::toggleLang(QTreeWidgetItem* item, int column)
{
    if (column != columnIdxCheckbox)
        return;

    dpsoSetLangIsActive(
        item->data(columnIdxCheckbox, Qt::UserRole).toInt(),
        item->checkState(columnIdxCheckbox) == Qt::Checked);
}


void LangBrowser::loadState(const DpsoCfg* cfg)
{
    const char* fallbackLangCode;
    if (dpsoGetNumLangs() == 1)
        fallbackLangCode = dpsoGetLangCode(0);
    else
        fallbackLangCode = dpsoGetDefaultLangCode();

    dpsoCfgLoadActiveLangs(cfg, cfgKeyOcrLanguages, fallbackLangCode);

    clear();
    setSortingEnabled(false);
    blockSignals(true);
    for (int i = 0; i < dpsoGetNumLangs(); ++i) {
        auto* item = new QTreeWidgetItem(this);

        item->setData(columnIdxCheckbox, Qt::UserRole, i);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(
            columnIdxCheckbox,
            dpsoGetLangIsActive(i) ? Qt::Checked : Qt::Unchecked);

        const auto* langCode = dpsoGetLangCode(i);
        const auto* langName = dpsoGetLangName(langCode);
        item->setText(
            columnIdxName,
            langName ? gettext(langName) : langCode);
        item->setText(columnIdxCode, langCode);
    }
    blockSignals(false);
    setSortingEnabled(true);

    const auto columnIdx = qBound(
        static_cast<int>(columnIdxName),
        dpsoCfgGetInt(
            cfg, cfgKeyUiLanguagesSortColumn, columnIdxName),
        static_cast<int>(columnIdxCode));

    Qt::SortOrder sortOrder;
    if (dpsoCfgGetBool(cfg, cfgKeyUiLanguagesSortDescending, false))
        sortOrder = Qt::DescendingOrder;
    else
        sortOrder = Qt::AscendingOrder;

    sortByColumn(columnIdx, sortOrder);
}


void LangBrowser::saveState(DpsoCfg* cfg) const
{
    dpsoCfgSetInt(
        cfg,
        cfgKeyUiLanguagesSortColumn,
        header()->sortIndicatorSection());
    dpsoCfgSetBool(
        cfg,
        cfgKeyUiLanguagesSortDescending,
        header()->sortIndicatorOrder() == Qt::DescendingOrder);

    dpsoCfgSaveActiveLangs(cfg, cfgKeyOcrLanguages);
}
