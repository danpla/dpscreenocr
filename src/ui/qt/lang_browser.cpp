
#include "lang_browser.h"

#include <QHeaderView>
#include <QTreeWidgetItem>

#include "dpso_intl/dpso_intl.h"
#include "ui_common/ui_common.h"


enum : int {
    columnIdxCheckbox,
    columnIdxName,
    columnIdxCode
};


// A custom item that allows sorting by the checkbox column.
class LangBrowserItem : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;

    bool operator<(const QTreeWidgetItem& other) const override
    {
        const auto* tw = treeWidget();
        Q_ASSERT(tw);

        if (tw->sortColumn() != columnIdxCheckbox)
            return QTreeWidgetItem::operator<(other);

        const auto thisCs = checkState(columnIdxCheckbox);
        const auto otherCs = other.checkState(columnIdxCheckbox);

        static_assert(Qt::Unchecked < Qt::PartiallyChecked, "");
        static_assert(Qt::PartiallyChecked < Qt::Checked, "");
        if (thisCs != otherCs)
            // In ascending order, checked items come first.
            return thisCs >= otherCs;

        // Within checkbox groups, always sort by name in ascending
        // order.

        const auto nameOrder = text(columnIdxName).localeAwareCompare(
            other.text(columnIdxName));

        if (tw->header()->sortIndicatorOrder() == Qt::AscendingOrder)
            return nameOrder < 0;

        return nameOrder >= 0;
    }
};


LangBrowser::LangBrowser(DpsoOcr* ocr, QWidget* parent)
    : QTreeWidget{parent}
    , ocr{ocr}
{
    setHeaderLabels({
        "",  // Checkbox
        pgettext("language", "Name"),
        pgettext("language", "Code")});

    setSortingEnabled(true);
    sortByColumn(columnIdxName, Qt::AscendingOrder);

    setAllColumnsShowFocus(true);
    setRootIsDecorated(false);

    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->setSectionsMovable(false);

    connect(
        this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
        this, SLOT(updateLangState(QTreeWidgetItem*, int)));

    // When the user enters QTreeWidget via keyboard (e.g. by pressing
    // Tab), the focus goes to the checkbox column, making it possible
    // to toggle the checkbox by pressing Space.
    //
    // However, it's also possible to set the focus to other columns
    // with a mouse click, and then pressing Space will have no effect
    // (setAllColumnsShowFocus(true) will show a focus rectangle for
    // the whole row, but will not prevent QTreeWidget to pick the
    // right column under the hood). To make keyboard interaction
    // consistent, we set the focus to the checkbox column each time
    // the user selects a new row.
    //
    // Another alternative would be to toggle the checkbox in response
    // to itemActivated(), but this signal is emitted on Enter rather
    // than on Space.
    connect(
        this, SIGNAL(
            currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
        this, SLOT(selectCheckboxColumn(QTreeWidgetItem*)));
}


void LangBrowser::updateLangState(QTreeWidgetItem* item, int column)
{
    if (column != columnIdxCheckbox)
        return;

    dpsoOcrSetLangIsActive(
        ocr,
        item->data(columnIdxCheckbox, Qt::UserRole).toInt(),
        item->checkState(columnIdxCheckbox) == Qt::Checked);
}


void LangBrowser::selectCheckboxColumn(QTreeWidgetItem* current)
{
    setCurrentItem(current, columnIdxCheckbox);
}


void LangBrowser::loadState(const DpsoCfg* cfg)
{
    const char* fallbackLangCode;
    if (dpsoOcrGetNumLangs(ocr) == 1)
        fallbackLangCode = dpsoOcrGetLangCode(ocr, 0);
    else
        fallbackLangCode = dpsoOcrGetDefaultLangCode(ocr);

    dpsoCfgLoadActiveLangs(
        cfg, cfgKeyOcrLanguages, ocr, fallbackLangCode);

    clear();
    setSortingEnabled(false);
    blockSignals(true);
    for (int i = 0; i < dpsoOcrGetNumLangs(ocr); ++i) {
        auto* item = new LangBrowserItem(this);

        item->setData(columnIdxCheckbox, Qt::UserRole, i);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(
            columnIdxCheckbox,
            dpsoOcrGetLangIsActive(ocr, i)
                ? Qt::Checked : Qt::Unchecked);

        const auto* langCode = dpsoOcrGetLangCode(ocr, i);
        const auto* langName = dpsoOcrGetLangName(ocr, langCode);
        item->setText(
            columnIdxName,
            langName ? gettext(langName) : langCode);
        item->setText(columnIdxCode, langCode);
    }
    blockSignals(false);
    setSortingEnabled(true);

    const auto columnIdx = qBound<int>(
        columnIdxCheckbox,
        dpsoCfgGetInt(
            cfg, cfgKeyUiLanguagesSortColumn, columnIdxName),
        columnIdxCode);

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

    dpsoCfgSaveActiveLangs(cfg, cfgKeyOcrLanguages, ocr);
}
