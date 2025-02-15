#include "lang_manager/lang_list_sort_filter_proxy.h"

#include "dpso_intl/dpso_intl.h"
#include "dpso_ocr/dpso_ocr.h"

#include "lang_manager/lang_list.h"
#include "lang_manager/metatypes.h"


namespace ui::qt::langManager {


LangListSortFilterProxy::LangListSortFilterProxy(
        LangGroup langGroup, QObject* parent)
    : QSortFilterProxyModel{parent}
    , langGroup{langGroup}
{
    setSortLocaleAware(true);
}


QVariant LangListSortFilterProxy::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    // LangGroup makes it clear what type of size is being displayed,
    // so specific header names like "Local size" from the base model
    // are redundant.
    if (role == Qt::DisplayRole
            && orientation == Qt::Horizontal
            && section == columnIdxSize)
        return pgettext("digital_data", "Size");

    return QSortFilterProxyModel::headerData(
        section, orientation, role);
}


void LangListSortFilterProxy::setFilterText(
    const QString& newFilterText)
{
    filterText = newFilterText;
    invalidateFilter();
}


bool LangListSortFilterProxy::filterAcceptsColumn(
    int sourceColumn, const QModelIndex& sourceParent) const
{
    (void)sourceParent;

    using LangGroup = LangListSortFilterProxy::LangGroup;

    return sourceColumn == LangList::columnIdxName
        || sourceColumn == LangList::columnIdxCode
        || (sourceColumn == LangList::columnIdxExternalSize
            && langGroup != LangGroup::removable)
        || (sourceColumn == LangList::columnIdxLocalSize
            && langGroup == LangGroup::removable);
}


static bool matchesGroup(
    DpsoOcrLangState state,
    LangListSortFilterProxy::LangGroup group)
{
    using LangGroup = LangListSortFilterProxy::LangGroup;

    switch (group) {
    case LangGroup::installable:
        return state == DpsoOcrLangStateNotInstalled;
    case LangGroup::updatable:
        return state == DpsoOcrLangStateUpdateAvailable;
    case LangGroup::removable:
        return
            state == DpsoOcrLangStateUpdateAvailable
            || state == DpsoOcrLangStateInstalled;
    }

    Q_ASSERT(false);
    return {};
}


bool LangListSortFilterProxy::filterAcceptsRow(
    int sourceRow, const QModelIndex& sourceParent) const
{
    const auto* sm = sourceModel();
    Q_ASSERT(sm);

    const auto langStateData = sm->data(
        sm->index(
            sourceRow, LangList::columnIdxState, sourceParent),
        Qt::UserRole);
    Q_ASSERT(langStateData.canConvert<DpsoOcrLangState>());

    if (!matchesGroup(
            langStateData.value<DpsoOcrLangState>(), langGroup))
        return false;

    if (filterText.isEmpty())
        return true;

    static const int filterableColumnIndices[]{
        LangList::columnIdxName, LangList::columnIdxCode
    };

    for (auto i : filterableColumnIndices)
        if (sm->data(sm->index(sourceRow, i, sourceParent)).
                toString().contains(
                    filterText, Qt::CaseInsensitive))
            return true;

    return false;
}


bool LangListSortFilterProxy::lessThan(
    const QModelIndex& sourceLeft,
    const QModelIndex& sourceRight) const
{
    const auto leftCol = sourceLeft.column();
    const auto rightCol = sourceRight.column();

    if (leftCol == rightCol
            && (leftCol == LangList::columnIdxExternalSize
                || leftCol == LangList::columnIdxLocalSize)) {
        const auto leftSize = sourceLeft.data(
            Qt::UserRole).value<int64_t>();
        const auto rightSize = sourceRight.data(
            Qt::UserRole).value<int64_t>();

        // If the sizes are the same, we could resort to sorting by
        // name, but that doesn't make much sense because the user
        // will see values rounded to kilobytes or more. That is, you
        // can't tell if several languages with "1.0 MB" had a
        // different byte size or were sorted by name.
        //
        // Instead, the stable sorting of QSortFilterProxyModel is
        // more convenient in practice.
        return leftSize < rightSize;
    }

    return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
}


}
