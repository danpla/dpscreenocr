
#pragma once

#include <QTreeWidget>

#include "dpso/dpso.h"
#include "dpso_ext/dpso_ext.h"


class QTreeWidgetItem;


class LangBrowser : public QTreeWidget {
    Q_OBJECT

public:
    explicit LangBrowser(DpsoOcr* ocr, QWidget* parent = nullptr);

    void loadState(const DpsoCfg* cfg);
    void saveState(DpsoCfg* cfg) const;
private slots:
    void updateLangState(QTreeWidgetItem* item, int column);
    void selectCheckboxColumn(QTreeWidgetItem* current);
private:
    DpsoOcr* ocr;
};
