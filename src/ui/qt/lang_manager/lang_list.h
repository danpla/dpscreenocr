#pragma once

#include <QAbstractTableModel>
#include <QList>

#include "dpso_ocr/dpso_ocr.h"


namespace ui::qt::langManager {


class LangList : public QAbstractTableModel {
    Q_OBJECT
public:
    enum {
        columnIdxName,
        columnIdxCode,
        columnIdxState,
        columnIdxExternalSize,
        columnIdxLocalSize,
    };

    explicit LangList(
        DpsoOcrLangManager* langManager, QObject* parent = nullptr);

    DpsoOcrLangManager* getLangManager();

    void reloadLangs();

    QVariant data(
        const QModelIndex& index,
        int role = Qt::DisplayRole) const override;

    QVariant headerData(
        int section,
        Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    int rowCount(
        const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(
        const QModelIndex& parent = QModelIndex()) const override;
private:
    DpsoOcrLangManager* langManager;

    struct LangInfo {
        QString name;
        QString code;
        DpsoOcrLangState state;
        DpsoOcrLangSize size;
    };
    QList<LangInfo> langInfos;
};


}
