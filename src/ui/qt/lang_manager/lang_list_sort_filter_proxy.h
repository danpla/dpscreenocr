
#pragma once

#include <QSortFilterProxyModel>


namespace langManager {


class LangListSortFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    enum class LangGroup {
        installable,
        updatable,
        removable
    };

    explicit LangListSortFilterProxy(
        LangGroup langGroup, QObject* parent = nullptr);

    bool filterAcceptsRow(
        int sourceRow,
        const QModelIndex& sourceParent) const override;
public slots:
    void setFilterText(const QString& newFilterText);
private:
    LangGroup langGroup;
    QString filterText;
};


}
