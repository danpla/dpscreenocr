
#include "lang_manager/lang_list.h"

#include "dpso_intl/dpso_intl.h"

#include "lang_manager/metatypes.h"
#include "utils.h"


namespace ui::qt::langManager {


LangList::LangList(
        DpsoOcrLangManager* langManager, QObject* parent)
    : QAbstractTableModel{parent}
    , langManager{langManager}
{
    reloadLangs();
}


DpsoOcrLangManager* LangList::getLangManager()
{
    return langManager;
}


void LangList::reloadLangs()
{
    beginResetModel();

    const auto numLangs = dpsoOcrLangManagerGetNumLangs(langManager);

    langInfos.clear();
    langInfos.reserve(numLangs);

    for (int i = 0; i < numLangs; ++i) {
        const auto* langCode =
            dpsoOcrLangManagerGetLangCode(langManager, i);
        const auto* langName =
            dpsoOcrLangManagerGetLangName(langManager, i);

        DpsoOcrLangSize langSize;
        dpsoOcrLangManagerGetLangSize(langManager, i, &langSize);

        langInfos.append({
            *langName ? gettext(langName) : langCode,
            langCode,
            dpsoOcrLangManagerGetLangState(langManager, i),
            langSize});
    }

    endResetModel();
}


QVariant LangList::data(const QModelIndex& index, int role) const
{
    const auto row = index.row();
    const auto col = index.column();

    Q_ASSERT(row < langInfos.size());
    const auto& langInfo = langInfos[row];

    if (role == Qt::DisplayRole) {
        switch (col) {
        case columnIdxName:
            return langInfo.name;
        case columnIdxCode:
            return langInfo.code;
        case columnIdxState:
            // This column is not shown to the user, so we don't care
            // how it's displayed.
            return static_cast<int>(langInfo.state);
        case columnIdxExternalSize:
            return formatDataSize(langInfo.size.external);
        case columnIdxLocalSize:
            return formatDataSize(langInfo.size.local);
        }

        Q_ASSERT(false);
        return {};
    } else if (role == Qt::UserRole)
        switch (col) {
        case columnIdxState:
            return QVariant::fromValue(langInfo.state);
        case columnIdxExternalSize:
            return QVariant::fromValue(langInfo.size.external);
        case columnIdxLocalSize:
            return QVariant::fromValue(langInfo.size.local);
        default:
            return {};
        }

    return {};
}


QVariant LangList::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};

    switch (section) {
    case columnIdxName:
        return pgettext("language", "Name");
    case columnIdxCode:
        return pgettext("language", "Code");
    case columnIdxState:
        // This column is not shown to the user, so don't bother with
        // translation.
        return "State";
    // Don't bother translating the size headers, because only the
    // external or local size will be displayed to the user, never
    // both. A proxy model will patch them as "Size".
    case columnIdxExternalSize:
        return "External size";
    case columnIdxLocalSize:
        return "Local size";
    }

    Q_ASSERT(false);
    return {};
}


int LangList::rowCount(const QModelIndex& parent) const
{
    (void)parent;
    return langInfos.size();
}


int LangList::columnCount(const QModelIndex& parent) const
{
    (void)parent;
    return 5;
}


}
