#pragma once

#include <QSortFilterProxyModel>


namespace ui::qt::langManager {


class LangListSortFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    enum {
        columnIdxName,
        columnIdxCode,
        columnIdxSize,
    };

    enum class LangGroup {
        installable,
        updatable,
        removable
    };

    explicit LangListSortFilterProxy(
        LangGroup langGroup, QObject* parent = nullptr);

    QVariant headerData(
        int section,
        Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;
public slots:
    void setFilterText(const QString& newFilterText);
protected:
    bool filterAcceptsColumn(
        int sourceColumn,
        const QModelIndex& sourceParent) const override;

    bool filterAcceptsRow(
        int sourceRow,
        const QModelIndex& sourceParent) const override;

    bool lessThan(
        const QModelIndex& sourceLeft,
        const QModelIndex& sourceRight) const override;
private:
    LangGroup langGroup;
    QString filterText;
};


}
