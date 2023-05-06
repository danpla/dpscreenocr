
#include "lang_manager/lang_list_sort_filter_proxy.h"

#include "lang_manager/lang_list.h"
#include "lang_manager/metatypes.h"

#include "dpso/dpso.h"


namespace langManager {


LangListSortFilterProxy::LangListSortFilterProxy(
        LangGroup langGroup, QObject* parent)
    : QSortFilterProxyModel{parent}
    , langGroup{langGroup}
{
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
            sourceRow, LangList::columnIdxName, sourceParent),
        Qt::UserRole);
    Q_ASSERT(langStateData.canConvert<DpsoOcrLangState>());

    if (!matchesGroup(
            langStateData.value<DpsoOcrLangState>(), langGroup))
        return false;

    if (filterText.isEmpty())
        return true;

    for (int i = 0; i < sm->columnCount(); ++i)
        if (sm->data(sm->index(sourceRow, i, sourceParent)).
                toString().contains(
                    filterText, Qt::CaseInsensitive))
            return true;

    return false;
}


void LangListSortFilterProxy::setFilterText(
    const QString& newFilterText)
{
    filterText = newFilterText;
    invalidateFilter();
}


}
