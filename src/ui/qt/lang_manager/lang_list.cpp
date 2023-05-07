
#include "lang_manager/lang_list.h"

#include "dpso_intl/dpso_intl.h"

#include "lang_manager/metatypes.h"


namespace langManager {


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

        langInfos.append({
            *langName ? gettext(langName) : langCode,
            langCode,
            dpsoOcrLangManagerGetLangState(langManager, i)
        });
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
        }

        Q_ASSERT(false);
        return {};
    } else if (role == Qt::UserRole && col == columnIdxName)
        return QVariant::fromValue(langInfo.state);

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
    return 2;
}


}
