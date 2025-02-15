#pragma once

#include <QTreeWidget>

#include "dpso_ext/dpso_ext.h"
#include "dpso_ocr/dpso_ocr.h"


class QTreeWidgetItem;


namespace ui::qt {


class LangBrowser : public QTreeWidget {
    Q_OBJECT
public:
    explicit LangBrowser(DpsoOcr* ocr);

    void reloadLangs();

    void loadState(const DpsoCfg* cfg);
    void saveState(DpsoCfg* cfg) const;
private slots:
    void updateLangState(QTreeWidgetItem* item, int column);
    void selectCheckboxColumn(QTreeWidgetItem* current);
private:
    DpsoOcr* ocr;
};


}
