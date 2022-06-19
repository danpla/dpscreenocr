
#pragma once

#include <QTreeWidget>

#include "dpso/dpso.h"
#include "dpso_utils/dpso_utils.h"


class QTreeWidgetItem;


class LangBrowser : public QTreeWidget {
    Q_OBJECT

public:
    explicit LangBrowser(DpsoOcr* ocr, QWidget* parent = nullptr);

    void loadState(const DpsoCfg* cfg);
    void saveState(DpsoCfg* cfg) const;
private slots:
    void toggleLang(QTreeWidgetItem* item, int column);
private:
    DpsoOcr* ocr;
};
