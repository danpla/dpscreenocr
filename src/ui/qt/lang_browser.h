#pragma once

#include <QTreeWidget>

#include "dpso_ext/dpso_ext.h"
#include "dpso_ocr/dpso_ocr.h"


namespace ui::qt {


class LangBrowser : public QTreeWidget {
    Q_OBJECT
public:
    explicit LangBrowser(DpsoOcr* ocr);

    void reloadLangs();

    void loadState(const DpsoCfg* cfg);
    void saveState(DpsoCfg* cfg) const;
private:
    DpsoOcr* ocr;
};


}
